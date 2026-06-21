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

   2次元配列は Mat クラスで表す (A(i,j) でアクセス, 中身は連続メモリ)。
   入力はNumPy標準の .npy 形式。I/O や行列演算は関数に分けてある (主眼は並列化)。 */

/* ====================== 2次元配列 (行列) ====================== */
/* 連続メモリの row-major 行列: A(i,j) = a[i*cols + j] */
struct Mat {
  long rows, cols;
  double * a;
  Mat(long r, long c)             : rows(r), cols(c), a(new double[r * c]) {}
  Mat(long r, long c, double * p) : rows(r), cols(c), a(p) {}   /* 既存バッファを引き取る */
  Mat(Mat && o) noexcept : rows(o.rows), cols(o.cols), a(o.a) { o.a = nullptr; }
  ~Mat() { delete[] a; }
  Mat(const Mat &) = delete;                  /* コピー禁止 (二重 delete を防ぐ) */
  Mat & operator=(const Mat &) = delete;
  double & operator()(long i, long j)       { return a[i * cols + j]; }
  double   operator()(long i, long j) const { return a[i * cols + j]; }
  double * data() { return a; }
};

/* ====================== .npy 入力 ====================== */
/* .npy を読み, 任意の数値型を double 配列に読み込む (C順, 1〜2次元) */
static double * read_npy(const char * path, long shape[2], int * ndim) {
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
  long s0 = 1, s1 = 1; *ndim = 1;
  char * sp = strstr(hdr, "shape"); sp = strchr(sp, '(') + 1;
  s0 = atol(sp);
  char * comma = strchr(sp, ',');
  char * after = comma + 1; while (*after == ' ') after++;
  if (*after != ')') { s1 = atol(after); *ndim = 2; } else { s1 = 1; *ndim = 1; }
  shape[0] = s0; shape[1] = s1;
  long n = s0 * (*ndim == 2 ? s1 : 1);
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

/* .npy を2次元の行列として読む (1次元なら cols=1) */
static Mat read_npy_mat(const char * path) {
  long sh[2]; int nd;
  double * a = read_npy(path, sh, &nd);
  return Mat(sh[0], (nd == 2 ? sh[1] : 1), a);
}

/* .npy を1次元のベクトルとして読む */
static double * read_npy_vec(const char * path) {
  long sh[2]; int nd;
  return read_npy(path, sh, &nd);
}

/* 画像 .npy (uint8 0..255) を読み, 0..1 に正規化した行列 [N,784] を返す */
static Mat load_images(const char * path) {
  Mat X = read_npy_mat(path);
  for (long i = 0; i < X.rows * X.cols; i++) X.data()[i] /= 255.0;
  return X;
}

/* ====================== 行列演算 ====================== */
/* 行列ベクトル積 + バイアス: y = W x + b  (W: rows×cols, x: cols, y: rows) */
static void matvec(const Mat & W, const double * b, const double * x, double * y) {
  for (long i = 0; i < W.rows; i++) {
    double s = b[i];
    for (long j = 0; j < W.cols; j++) s += W(i, j) * x[j];
    y[i] = s;
  }
}

static void relu_inplace(double * v, long n) {
  for (long i = 0; i < n; i++) if (v[i] < 0.0) v[i] = 0.0;
}

static int argmax(const double * v, long n) {
  int best = 0;
  for (long i = 1; i < n; i++) if (v[i] > v[best]) best = (int)i;
  return best;
}

/* 1枚の画像を MLP に通して予測クラスを返す: h=ReLU(W1 x+b1), o=W2 h+b2, argmax(o) */
static int predict(const Mat & W1, const double * b1,
                   const Mat & W2, const double * b2, const double * x) {
  double h[1024], o[64];               /* HID<=1024, OUT<=64 を仮定 */
  matvec(W1, b1, x, h); relu_inplace(h, W1.rows);
  matvec(W2, b2, h, o);
  return argmax(o, W2.rows);
}

/* ====================== main ====================== */
int main(int argc, char ** argv) {
  /* 学習済みの重みとテスト画像を読み込む */
  Mat W1 = read_npy_mat("data/W1.npy");      /* [HID, IN]  */
  Mat W2 = read_npy_mat("data/W2.npy");      /* [OUT, HID] */
  double * b1 = read_npy_vec("data/b1.npy");
  double * b2 = read_npy_vec("data/b2.npy");
  Mat X = load_images("data/x_test.npy");    /* [NT, IN], 0..1 正規化済み */
  double * y = read_npy_vec("data/y_test.npy");
  long NT = X.rows;

  /* 推論: 各画像の予測クラスと正解ラベルを比べ, 正解数を数える。各画像は独立。 */
  long correct = 0;
  double t0 = omp_get_wtime();
  // BEGIN ANSWER: 各画像の推論は独立。#pragma omp parallel for reduction(+:correct) で並列化せよ.
#pragma omp parallel for reduction(+:correct)
  // END ANSWER
  for (long i = 0; i < NT; i++)
    if (predict(W1, b1, W2, b2, &X(i, 0)) == (int)y[i]) correct++;
  double elapsed = omp_get_wtime() - t0;

  printf("MNIST テスト %ld 枚: 正解 %ld 枚, 正解率 = %.2f%%\n",
         NT, correct, 100.0 * correct / NT);
  printf("elapsed = %.3f sec\n", elapsed);
  delete[] b1; delete[] b2; delete[] y;       /* W1,W2,X は Mat のデストラクタが解放 */
  return 0;
}
