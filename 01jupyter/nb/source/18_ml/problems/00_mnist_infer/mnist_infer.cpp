#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
   - data/mnist_weights.txt : 学習済みの重み (784->128->10)
   - data/mnist_test.txt    : テスト画像 (28x28=784画素, 0..255) と正解ラベル
   推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」。これまで並列化してきた
   行列計算が, そのまま手書き数字の認識になる。各画像の推論は独立なので並列化できる。 */
int main(int argc, char ** argv) {
  int IN, HID, OUT;

  /* --- 重みの読み込み --- */
  FILE * fw = fopen("data/mnist_weights.txt", "r");
  if (!fw) { printf("data/mnist_weights.txt が開けません\n"); return 1; }
  if (fscanf(fw, "%d %d %d", &IN, &HID, &OUT) != 3) return 1;
  double * W1 = (double *)malloc(sizeof(double) * HID * IN);
  double * b1 = (double *)malloc(sizeof(double) * HID);
  double * W2 = (double *)malloc(sizeof(double) * OUT * HID);
  double * b2 = (double *)malloc(sizeof(double) * OUT);
  for (long k = 0; k < (long)HID * IN; k++)  fscanf(fw, "%lf", &W1[k]);
  for (int k = 0; k < HID; k++)              fscanf(fw, "%lf", &b1[k]);
  for (long k = 0; k < (long)OUT * HID; k++) fscanf(fw, "%lf", &W2[k]);
  for (int k = 0; k < OUT; k++)              fscanf(fw, "%lf", &b2[k]);
  fclose(fw);

  /* --- テスト画像の読み込み (画素 0..255 -> 0..1 に正規化) --- */
  FILE * ft = fopen("data/mnist_test.txt", "r");
  if (!ft) { printf("data/mnist_test.txt が開けません\n"); return 1; }
  int NT, IN2;
  if (fscanf(ft, "%d %d", &NT, &IN2) != 2) return 1;
  double * X = (double *)malloc(sizeof(double) * (long)NT * IN);
  int *    y = (int *)malloc(sizeof(int) * NT);
  for (int i = 0; i < NT; i++) {
    for (int k = 0; k < IN; k++) { int v; fscanf(ft, "%d", &v); X[(long)i*IN+k] = v / 255.0; }
    fscanf(ft, "%d", &y[i]);
  }
  fclose(ft);

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
