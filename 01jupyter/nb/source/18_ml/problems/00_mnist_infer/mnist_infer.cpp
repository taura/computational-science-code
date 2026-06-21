#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <omp.h>

/* 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
   重みは 02_mlp_train が学習して書き出したもの (784->128->10):
   - data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
   - data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
   - data/y_test.npy : 正解ラベル (int32 [N], 0..9)
   推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」。これまで並列化してきた
   行列計算が, そのまま手書き数字の認識になる。各画像の推論は独立なので並列化できる。

   2次元配列は下の Mat クラスで表す (A(i,j) でアクセス, 中身は連続メモリ)。
   入力はNumPy標準の .npy 形式。read_npy も下に用意してある (I/O は主眼ではない)。 */

/* ---- 連続メモリの2次元配列 (row-major: A(i,j) = a[i*cols + j]) ---- */
struct Mat {
  long rows, cols;
  double * a;
  Mat(long r, long c)             : rows(r), cols(c), a(new double[r * c]) {}
  Mat(long r, long c, double * p) : rows(r), cols(c), a(p) {}   /* 既存バッファを引き取る */
  ~Mat() { delete[] a; }
  Mat(const Mat &) = delete;                  /* コピー禁止 (二重 delete を防ぐ) */
  Mat & operator=(const Mat &) = delete;
  double & operator()(long i, long j)       { return a[i * cols + j]; }
  double   operator()(long i, long j) const { return a[i * cols + j]; }
  double * data() { return a; }
};

/* ---- .npy 読み込み: 任意の数値型を double 配列に読み込む (C順, 1〜2次元) ---- */
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

int main(int argc, char ** argv) {
  long sh[2]; int nd;

  /* --- 学習済みの重みの読み込み (2次元は Mat, 1次元は double[] ) --- */
  long shw[2];
  double * w1 = read_npy("data/W1.npy", shw, &nd); int HID = shw[0], IN = shw[1];
  Mat W1(HID, IN, w1);
  double * b1 = read_npy("data/b1.npy", sh, &nd);
  double * w2 = read_npy("data/W2.npy", shw, &nd); int OUT = shw[0];
  Mat W2(OUT, HID, w2);
  double * b2 = read_npy("data/b2.npy", sh, &nd);

  /* --- テスト画像の読み込み (画素 0..255 -> 0..1 に正規化) --- */
  double * xr = read_npy("data/x_test.npy", sh, &nd); int NT = sh[0];
  for (long i = 0; i < (long)NT * IN; i++) xr[i] /= 255.0;
  Mat X(NT, IN, xr);
  double * yd = read_npy("data/y_test.npy", sh, &nd);
  int * y = new int[NT];
  for (int i = 0; i < NT; i++) y[i] = (int)yd[i];
  delete[] yd;

  /* --- 推論: 各画像を MLP に通して予測クラス(argmax)を求め, 正解数を数える --- */
  long correct = 0;
  double t0 = omp_get_wtime();
  // BEGIN ANSWER: 各画像の推論は独立。#pragma omp parallel for reduction(+:correct) で並列化せよ.
#pragma omp parallel for reduction(+:correct)
  // END ANSWER
  for (int i = 0; i < NT; i++) {
    double h[1024];                       /* 隠れ層 (HID<=1024 を仮定) */
    for (int hh = 0; hh < HID; hh++) {    /* h = ReLU(W1 x + b1) */
      double s = b1[hh];
      for (int k = 0; k < IN; k++) s += W1(hh, k) * X(i, k);
      h[hh] = (s > 0.0) ? s : 0.0;
    }
    int best = 0; double bestv = -1e300;  /* o = W2 h + b2, argmax */
    for (int oo = 0; oo < OUT; oo++) {
      double s = b2[oo];
      for (int hh = 0; hh < HID; hh++) s += W2(oo, hh) * h[hh];
      if (s > bestv) { bestv = s; best = oo; }
    }
    if (best == y[i]) correct++;
  }
  double elapsed = omp_get_wtime() - t0;

  printf("MNIST テスト %d 枚: 正解 %ld 枚, 正解率 = %.2f%%\n",
         NT, correct, 100.0 * correct / NT);
  printf("elapsed = %.3f sec\n", elapsed);
  delete[] b1; delete[] b2; delete[] y;     /* W1,W2,X は Mat のデストラクタが解放 */
  return 0;
}
