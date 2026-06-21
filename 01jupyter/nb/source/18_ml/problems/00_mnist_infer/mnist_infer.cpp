#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <omp.h>

/* 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
   重みは 02_mlp_train が学習して書き出したもの (784->128->10):
   - data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
   - data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
   - data/y_test.npy : 正解ラベル (int32 [N], 0..9)
   推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」(下の matvec / predict)。
   各画像の推論は独立なので並列化できる。

   ネットワークの大きさは定数 (IN/HID/OUT) なので, 全パラメータを固定サイズ配列で
   1つの構造体 Net にまとめる (ヒープ確保なし, 本物の2次元配列 W1[k][j])。Net は
   内部にポインタを持たないので, GPU 発展では #pragma omp target map(to: net) で
   モデルをまるごとデバイスへ送れる。 */

static const int IN = 784, HID = 128, OUT = 10;

struct Net {
  double W1[HID][IN], b1[HID], W2[OUT][HID], b2[OUT];
};

/* ====================== .npy 入力 ====================== */
/* .npy を読み, 任意の数値型を double 配列に読み込む (C順)。次元数 ndim は呼び出し側が
   指定し, ファイルの次元がそれと違えばエラーにする。shape には ndim 個の要素を格納する。 */
static double * read_npy(const char * path, long * shape, int ndim) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);           /* ヘッダ(辞書文字列)の長さ */
  char * hdr = new char[hlen + 1];
  fread(hdr, 1, hlen, f); hdr[hlen] = '\0';
  /* dtype (例: '<f8','|u1','<i4','<i8') と shape をヘッダ文字列から取り出す */
  char descr[8] = {0};
  { char * q = strstr(hdr, "descr"); q = strchr(q, ':'); q = strchr(q, '\'') + 1;
    int i = 0; while (*q != '\'' && i < 7) descr[i++] = *q++; descr[i] = '\0'; }
  long s0 = 1, s1 = 1; int file_ndim;
  char * sp = strstr(hdr, "shape"); sp = strchr(sp, '(') + 1;
  s0 = atol(sp);
  char * comma = strchr(sp, ',');
  char * after = comma + 1; while (*after == ' ') after++;
  if (*after != ')') { s1 = atol(after); file_ndim = 2; } else { file_ndim = 1; }
  if (file_ndim != ndim) {
    printf("%s: %d 次元を期待しましたが %d 次元でした\n", path, ndim, file_ndim); exit(1);
  }
  shape[0] = s0;
  long n = s0;
  if (ndim == 2) { shape[1] = s1; n *= s1; }
  delete[] hdr;

  double * out = new double[n];
  if (!strcmp(descr, "<f8")) {
    fread(out, sizeof(double), n, f);
  } else if (!strcmp(descr, "<f4")) {
    float * t = new float[n]; fread(t, sizeof(float), n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "|u1")) {
    unsigned char * t = new unsigned char[n]; fread(t, 1, n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i4")) {
    int * t = new int[n]; fread(t, sizeof(int), n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i8")) {
    long long * t = new long long[n]; fread(t, sizeof(long long), n, f);
    for (long i = 0; i < n; i++) { out[i] = (double)t[i]; } delete[] t;
  } else {
    printf("%s: 未対応の dtype %s\n", path, descr); exit(1);
  }
  fclose(f);
  return out;
}

/* .npy を固定サイズ配列 dst に読み込む (形が (n0,n1) と違えばエラー)。n1=0 なら1次元。 */
static void load_npy(const char * path, double * dst, long n0, long n1) {
  long sh[2];
  double * p = read_npy(path, sh, (n1 > 0 ? 2 : 1));
  if (sh[0] != n0 || (n1 > 0 && sh[1] != n1)) {
    printf("%s: 形が想定 (%ld,%ld) と違います\n", path, n0, n1); exit(1);
  }
  long n = n0 * (n1 > 0 ? n1 : 1);
  for (long i = 0; i < n; i++) dst[i] = p[i];
  delete[] p;
}

/* ====================== 行列演算 ====================== */
/* 行列ベクトル積 + バイアス: y = W x + b  (W は R×C, x は C, y と b は R)。
   W のサイズ R,C は配列の型から自動で決まる (テンプレート)。 */
template <int R, int C>
static void matvec(const double (&W)[R][C], const double * x,
                   const double (&b)[R], double (&y)[R]) {
  for (int i = 0; i < R; i++) {
    double s = b[i];
    for (int j = 0; j < C; j++) s += W[i][j] * x[j];
    y[i] = s;
  }
}

static void relu_inplace(double * v, int n) {
  for (int i = 0; i < n; i++) if (v[i] < 0.0) v[i] = 0.0;
}

static int argmax(const double * v, int n) {
  int best = 0;
  for (int i = 1; i < n; i++) if (v[i] > v[best]) best = i;
  return best;
}

/* 1枚の画像 x を MLP に通して予測クラスを返す: h=ReLU(W1 x+b1), o=W2 h+b2, argmax(o) */
static int predict(const Net & net, const double * x) {
  double h[HID], o[OUT];
  matvec(net.W1, x, net.b1, h); relu_inplace(h, HID);
  matvec(net.W2, h, net.b2, o);
  return argmax(o, OUT);
}

/* ====================== main ====================== */
int main(int argc, char ** argv) {
  /* 学習済みの重みを固定サイズの Net に読み込む */
  Net net;
  load_npy("data/W1.npy", &net.W1[0][0], HID, IN);
  load_npy("data/b1.npy", net.b1, HID, 0);
  load_npy("data/W2.npy", &net.W2[0][0], OUT, HID);
  load_npy("data/b2.npy", net.b2, OUT, 0);

  /* テスト画像 (枚数 NT は可変なので動的配列) を読み, 画素を 0..1 に正規化 */
  long sh[2];
  double * X = read_npy("data/x_test.npy", sh, 2);   /* [NT, IN] */
  int NT = sh[0];
  for (long i = 0; i < (long)NT * IN; i++) X[i] /= 255.0;
  double * yd = read_npy("data/y_test.npy", sh, 1);
  int * y = new int[NT];
  for (int i = 0; i < NT; i++) y[i] = (int)yd[i];
  delete[] yd;

  /* 推論: 各画像の予測クラスと正解ラベルを比べ, 正解数を数える。各画像は独立。 */
  long correct = 0;
  double t0 = omp_get_wtime();
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:correct)
  // END ANSWER
  for (int i = 0; i < NT; i++)
    if (predict(net, &X[(long)i * IN]) == y[i]) correct++;
  double elapsed = omp_get_wtime() - t0;

  printf("MNIST テスト %d 枚: 正解 %ld 枚, 正解率 = %.2f%%\n",
         NT, correct, 100.0 * correct / NT);
  printf("elapsed = %.3f sec\n", elapsed);
  delete[] X; delete[] y;
  return 0;
}
