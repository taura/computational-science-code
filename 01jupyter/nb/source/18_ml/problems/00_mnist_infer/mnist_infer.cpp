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

   入力はNumPy標準の .npy 形式 (ヘッダ + 生バイナリ)。read_npy は下に用意してある
   (I/O は本問題の主眼ではない)。 */

/* ---- .npy 読み込み: 任意の数値型を double 配列に読み込む (C順, 1〜2次元) ---- */
static double * read_npy(const char * path, long shape[2], int * ndim) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);           /* ヘッダ(辞書文字列)の長さ */
  char * hdr = (char *)malloc(hlen + 1);
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
  free(hdr);

  double * out = (double *)malloc(sizeof(double) * n);
  if (!strcmp(descr, "<f8")) {
    fread(out, sizeof(double), n, f);
  } else if (!strcmp(descr, "<f4")) {
    float * t = (float *)malloc(sizeof(float) * n); fread(t, sizeof(float), n, f);
    for (long i = 0; i < n; i++) out[i] = t[i]; free(t);
  } else if (!strcmp(descr, "|u1")) {
    unsigned char * t = (unsigned char *)malloc(n); fread(t, 1, n, f);
    for (long i = 0; i < n; i++) out[i] = t[i]; free(t);
  } else if (!strcmp(descr, "<i4")) {
    int * t = (int *)malloc(sizeof(int) * n); fread(t, sizeof(int), n, f);
    for (long i = 0; i < n; i++) out[i] = t[i]; free(t);
  } else if (!strcmp(descr, "<i8")) {
    long long * t = (long long *)malloc(sizeof(long long) * n); fread(t, sizeof(long long), n, f);
    for (long i = 0; i < n; i++) out[i] = (double)t[i]; free(t);
  } else {
    printf("%s: 未対応の dtype %s\n", path, descr); exit(1);
  }
  fclose(f);
  return out;
}

int main(int argc, char ** argv) {
  long sh[2]; int nd;

  /* --- 学習済みの重みの読み込み --- */
  double * W1 = read_npy("data/W1.npy", sh, &nd); int HID = sh[0], IN = sh[1];
  double * b1 = read_npy("data/b1.npy", sh, &nd);
  double * W2 = read_npy("data/W2.npy", sh, &nd); int OUT = sh[0];
  double * b2 = read_npy("data/b2.npy", sh, &nd);

  /* --- テスト画像の読み込み (画素 0..255 -> 0..1 に正規化) --- */
  double * Xu = read_npy("data/x_test.npy", sh, &nd); int NT = sh[0];
  double * yd = read_npy("data/y_test.npy", sh, &nd);
  double * X = (double *)malloc(sizeof(double) * (long)NT * IN);
  int    * y = (int *)malloc(sizeof(int) * NT);
  for (long i = 0; i < (long)NT * IN; i++) X[i] = Xu[i] / 255.0;
  for (int i = 0; i < NT; i++)             y[i] = (int)yd[i];
  free(Xu); free(yd);

  /* --- 推論: 各画像を MLP に通して予測クラス(argmax)を求め, 正解数を数える --- */
  long correct = 0;
  double t0 = omp_get_wtime();
  // BEGIN ANSWER: 各画像の推論は独立。#pragma omp parallel for reduction(+:correct) で並列化せよ.
#pragma omp parallel for reduction(+:correct)
  // END ANSWER
  for (int i = 0; i < NT; i++) {
    double h[1024];                       /* 隠れ層 (HID<=1024 を仮定) */
    const double * x = &X[(long)i * IN];
    for (int hh = 0; hh < HID; hh++) {    /* h = ReLU(W1 x + b1) */
      double s = b1[hh];
      const double * w = &W1[(long)hh * IN];
      for (int k = 0; k < IN; k++) s += w[k] * x[k];
      h[hh] = (s > 0.0) ? s : 0.0;
    }
    int best = 0; double bestv = -1e300;  /* o = W2 h + b2, argmax */
    for (int oo = 0; oo < OUT; oo++) {
      double s = b2[oo];
      const double * w = &W2[(long)oo * HID];
      for (int hh = 0; hh < HID; hh++) s += w[hh] * h[hh];
      if (s > bestv) { bestv = s; best = oo; }
    }
    if (best == y[i]) correct++;
  }
  double elapsed = omp_get_wtime() - t0;

  printf("MNIST テスト %d 枚: 正解 %ld 枚, 正解率 = %.2f%%\n",
         NT, correct, 100.0 * correct / NT);
  printf("elapsed = %.3f sec\n", elapsed);
  free(W1); free(b1); free(W2); free(b2); free(X); free(y);
  return 0;
}
