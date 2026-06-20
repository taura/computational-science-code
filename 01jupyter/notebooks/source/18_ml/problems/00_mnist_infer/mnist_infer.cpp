#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (重み・入力の生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* MNIST を模した 2層 MLP の推論 (forward):
   入力 784 → 隠れ 128 (ReLU) → 出力 10。
   h = ReLU(W1 x + b1)  (128次元), o = W2 h + b2 (10次元), 予測 = argmax(o)。
   ニューラルネットの推論の正体は「行列積 + 活性化関数」であり,
   これまで並列化してきた行列(ベクトル)積がそのまま AI の推論になる。
   重みは乱数 (学習済みパラメータの代わり) なので予測の中身に意味はないが,
   計算の流れ (行列積 + ReLU + argmax) は本物である。 */

#define IN  784   /* 入力次元 */
#define HID 128   /* 隠れ層のニューロン数 */
#define OUT 10    /* 出力クラス数 */

int main(int argc, char ** argv) {
  int B = (argc > 1 ? atoi(argv[1]) : 64);     /* バッチサイズ (画像枚数) */
  int R = (argc > 2 ? atoi(argv[2]) : 2000);   /* 繰り返し回数 (計測を有意にするため) */

  /* 重み・バイアスを乱数で生成 (小さい値 [-0.05, 0.05) 付近)。 */
  double * W1 = (double *)malloc(sizeof(double) * HID * IN);
  double * b1 = (double *)malloc(sizeof(double) * HID);
  double * W2 = (double *)malloc(sizeof(double) * OUT * HID);
  double * b2 = (double *)malloc(sizeof(double) * OUT);
  for (long i = 0; i < (long)HID * IN; i++) W1[i] = (draw_rand01(i, 1) - 0.5) * 0.1;
  for (long i = 0; i < HID; i++)            b1[i] = (draw_rand01(i, 2) - 0.5) * 0.1;
  for (long i = 0; i < (long)OUT * HID; i++) W2[i] = (draw_rand01(i, 3) - 0.5) * 0.1;
  for (long i = 0; i < OUT; i++)            b2[i] = (draw_rand01(i, 4) - 0.5) * 0.1;

  /* バッチの入力「画像」B 枚 (各 784 個の数値) を乱数で生成。 */
  double * X = (double *)malloc(sizeof(double) * (long)B * IN);
  for (long i = 0; i < (long)B * IN; i++) X[i] = draw_rand01(i, 5);

  int * pred = (int *)malloc(sizeof(int) * B);
  double checksum = 0.0;

  double t0 = omp_get_wtime();
  for (int rep = 0; rep < R; rep++) {
    checksum = 0.0;
    /* 各画像の forward は互いに独立。バッチを分担して並列に推論する。 */
    // TODO: バッチ (画像) のループを #pragma omp parallel for reduction(+:checksum) で並列化せよ.
    for (int n = 0; n < B; n++) {
      const double * x = X + (long)n * IN;
      double h[HID];
      double o[OUT];
      /* 1層目: h = ReLU(W1 x + b1)  (行列ベクトル積 + ReLU) */
      for (int j = 0; j < HID; j++) {
        double s = b1[j];
        for (int k = 0; k < IN; k++) s += W1[(long)j * IN + k] * x[k];
        h[j] = (s > 0.0 ? s : 0.0);   /* ReLU */
      }
      /* 2層目: o = W2 h + b2  (行列ベクトル積) */
      for (int c = 0; c < OUT; c++) {
        double s = b2[c];
        for (int k = 0; k < HID; k++) s += W2[(long)c * HID + k] * h[k];
        o[c] = s;
        checksum += s;
      }
      /* 予測クラス = argmax(o) */
      int amax = 0;
      for (int c = 1; c < OUT; c++) if (o[c] > o[amax]) amax = c;
      pred[n] = amax;
    }
  }
  double elapsed = omp_get_wtime() - t0;

  int show = (B < 8 ? B : 8);
  printf("batch=%d, hidden=%d: 予測クラス[0..%d]=", B, HID, show - 1);
  for (int n = 0; n < show; n++) printf("%d%s", pred[n], n + 1 < show ? "," : "");
  printf(", checksum=%.6f\n", checksum);
  printf("(結果は OMP_NUM_THREADS によらず一致する: 各画像は固定順序で独立に計算)\n");
  printf("elapsed = %.3f sec\n", elapsed);
  free(W1); free(b1); free(W2); free(b2); free(X); free(pred);
  return 0;
}
