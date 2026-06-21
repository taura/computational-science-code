#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <omp.h>

/* 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
   重みは 02_mlp_train が学習して書き出したもの (784->128->10):
   - data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
   - data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
   - data/y_test.npy : 正解ラベル (int32 [N], 0..9)

   テスト画像 NT 枚を**まとめて行列として**前向き計算する:
     H = ReLU(W1 X + b1),  P = softmax(W2 H + b2)        (X:[NT,IN], H:[NT,HID], P:[NT,OUT])
   行列積 (matmul_bias) を活性化 (relu/softmax) から分けてある。matmul_bias はバッチ中の
   全サンプルを一度に処理し, 行(=サンプル)ごとに独立なのでその行ループを並列化できる。

   ネットワークの大きさは定数なので, 重み・画像・ラベル・中間結果をすべて固定サイズ配列で
   1つの構造体 Net にまとめる。Net は内部にポインタを持たないので, GPU 発展では
   map(to: net) でまるごと送れる。 */

static const int IN = 784, HID = 128, OUT = 10;
static const int MAXN = 10000;          /* テスト画像の最大枚数 (これを超える分は読まない) */

struct Net {
  double W1[HID][IN], b1[HID], W2[OUT][HID], b2[OUT];   /* 学習済みの重み */
  double X[MAXN][IN], y[MAXN];                          /* テスト画像と正解ラベル */
  double H[MAXN][HID], P[MAXN][OUT];                    /* 中間: 隠れ層, 出力確率 (バッチ全体) */
};

/* ====================== .npy 入力 ====================== */
/* .npy を読み, 任意の数値型を double として dst に直接書き込む (C順)。次元数 ndim は
   呼び出し側が指定し, ファイルの次元が違えばエラー。形を shape[0..ndim-1] に返す。
   dst=nullptr なら形だけ取得 (peek)。maxrows>=0 なら先頭 maxrows 行までしか読まない。 */
static void read_npy(const char * path, double * dst, long * shape, int ndim, long maxrows = -1) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);
  char * hdr = new char[hlen + 1];
  fread(hdr, 1, hlen, f); hdr[hlen] = '\0';
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
  if (ndim == 2) shape[1] = s1;
  delete[] hdr;
  if (dst == nullptr) { fclose(f); return; }

  long rows = (maxrows >= 0 && s0 > maxrows) ? maxrows : s0;
  long n = rows * (ndim == 2 ? s1 : 1);
  if (!strcmp(descr, "<f8")) {
    fread(dst, sizeof(double), n, f);
  } else if (!strcmp(descr, "<f4")) {
    float * t = new float[n]; fread(t, sizeof(float), n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "|u1")) {
    unsigned char * t = new unsigned char[n]; fread(t, 1, n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i4")) {
    int * t = new int[n]; fread(t, sizeof(int), n, f);
    for (long i = 0; i < n; i++) { dst[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i8")) {
    long long * t = new long long[n]; fread(t, sizeof(long long), n, f);
    for (long i = 0; i < n; i++) { dst[i] = (double)t[i]; } delete[] t;
  } else {
    printf("%s: 未対応の dtype %s\n", path, descr); exit(1);
  }
  fclose(f);
}

/* read_npy + 形のチェックだけ。容量 cap 行までを dst に読み, 実際に読んだ行数を返す。
   n1=0 なら1次元, n1>0 なら列数を確認。 */
static long load_npy(const char * path, double * dst, long cap, long n1) {
  long sh[2];
  read_npy(path, dst, sh, (n1 > 0 ? 2 : 1), cap);
  if (n1 > 0 && sh[1] != n1) { printf("%s: 列数が想定 %ld と違います (%ld)\n", path, n1, sh[1]); exit(1); }
  return (sh[0] > cap) ? cap : sh[0];
}

/* ====================== バッチ行列演算 (各ステップが m 枚を一度に処理) ====================== */
/* 行列積 + バイアス: Y = X W^T + b   (W:R×C, X:m×C, Y:m×R)。行 i (サンプル) ごとに独立。
   これが「AI の計算の中身」である行列積。重いのでここを並列化する。 */
template <int R, int C>
static void matmul_bias(const double (&W)[R][C], const double X[][C],
                   const double (&b)[R], double Y[][R], int m) {
  // TODO: 行 i (サンプル) のループを並列化する (各行は独立)。
  // BEGIN ANSWER
#pragma omp parallel for
  // END ANSWER
  for (int i = 0; i < m; i++)
    for (int k = 0; k < R; k++) {
      double s = b[k];
      for (int j = 0; j < C; j++) s += W[k][j] * X[i][j];
      Y[i][k] = s;
    }
}

/* 活性化: バッチの全要素に適用する軽い処理 (要素ごとなので逐次でよい) */
template <int N> static void relu(double Y[][N], int m) {
  for (int i = 0; i < m; i++)
    for (int k = 0; k < N; k++) if (Y[i][k] < 0.0) Y[i][k] = 0.0;
}
template <int N> static void softmax(double Y[][N], int m) {
  for (int i = 0; i < m; i++) {
    double mx = Y[i][0];
    for (int k = 1; k < N; k++) if (Y[i][k] > mx) mx = Y[i][k];
    double s = 0.0;
    for (int k = 0; k < N; k++) { Y[i][k] = exp(Y[i][k] - mx); s += Y[i][k]; }
    for (int k = 0; k < N; k++) Y[i][k] /= s;
  }
}

static int argmax(const double * v, int n) {
  int best = 0;
  for (int i = 1; i < n; i++) if (v[i] > v[best]) best = i;
  return best;
}

/* バッチ全体を前向き計算: H = ReLU(W1 X + b1), P = softmax(W2 H + b2) */
static void forward(Net & net, int m) {
  matmul_bias(net.W1, net.X, net.b1, net.H, m); relu(net.H, m);    /* H = ReLU(W1 X + b1)   */
  matmul_bias(net.W2, net.H, net.b2, net.P, m); softmax(net.P, m); /* P = softmax(W2 H + b2) */
}

/* 予測クラス (argmax P) と正解ラベルを比べ, 正解数を返す */
static long count_correct(Net & net, int m) {
  long correct = 0;
  // TODO: 各画像の判定を並列化して正解数を集計する (各行は独立)。
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:correct)
  // END ANSWER
  for (int i = 0; i < m; i++)
    if (argmax(net.P[i], OUT) == (int)net.y[i]) correct++;
  return correct;
}

/* ====================== main ====================== */
static Net net;          /* 画像・中間も含み大きいので静的領域に置く */

int main(int argc, char ** argv) {
  load_npy("data/W1.npy", &net.W1[0][0], HID, IN);
  load_npy("data/b1.npy", net.b1, HID, 0);
  load_npy("data/W2.npy", &net.W2[0][0], OUT, HID);
  load_npy("data/b2.npy", net.b2, OUT, 0);
  int NT = (int)load_npy("data/x_test.npy", &net.X[0][0], MAXN, IN);
  load_npy("data/y_test.npy", net.y, MAXN, 0);
  for (long k = 0; k < (long)NT * IN; k++) (&net.X[0][0])[k] /= 255.0;   /* 0..255 -> 0..1 */

  /* 全画像をまとめて forward し, 正解数を数える */
  double t0 = omp_get_wtime();
  forward(net, NT);
  long correct = count_correct(net, NT);
  double elapsed = omp_get_wtime() - t0;

  printf("MNIST テスト %d 枚: 正解 %ld 枚, 正解率 = %.2f%%\n",
         NT, correct, 100.0 * correct / NT);
  printf("elapsed = %.3f sec\n", elapsed);
  return 0;
}
