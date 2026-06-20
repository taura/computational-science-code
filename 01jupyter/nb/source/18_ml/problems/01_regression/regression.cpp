#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (合成データ生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

static inline double sigmoid(double z) { return 1.0 / (1.0 + exp(-z)); }

/* 勾配降下法でロジスティック回帰を学習する。
   予測 p = sigmoid(w・x)。損失は二値クロスエントロピー。
   勾配 grad[d] = (1/N) Σ_i (sigmoid(w・x_i) - y_i) * x_i[d]。
   w を 0 から始め, 各エポックで w[d] -= lr * grad[d] と更新する。
   合成データは線形分離可能なので, 学習が進むと正解率が上がっていくのを観察できる。
   並列化対象は「全サンプルにわたる予測・誤差の和」(行列積 + reduction)。 */
int main(int argc, char ** argv) {
  long N = (argc > 1 ? atol(argv[1]) : 200000);  /* サンプル数 */
  int  D = (argc > 2 ? atoi(argv[2]) : 20);      /* 特徴次元 */
  int  E = (argc > 3 ? atoi(argv[3]) : 200);     /* エポック数 */
  double lr = (argc > 4 ? atof(argv[4]) : 1.0);  /* 学習率 */

  /* 真の重み w_true (= 学習で復元したい正解), 範囲 [-1,1)。 */
  double * w_true = (double *)malloc(sizeof(double) * D);
  for (int d = 0; d < D; d++) w_true[d] = draw_rand01(d, 7) * 2.0 - 1.0;

  /* 特徴 x[i][d] (中心化), ラベル y[i] = (w_true・x_i > 0)。線形分離可能。 */
  double * X = (double *)malloc(sizeof(double) * N * D);
  int    * y = (int *)malloc(sizeof(int) * N);
  for (long i = 0; i < N; i++) {
    double score = 0.0;
    for (int d = 0; d < D; d++) {
      double xv = draw_rand01(i, d) - 0.5;
      X[i * D + d] = xv;
      score += w_true[d] * xv;
    }
    y[i] = (score > 0.0) ? 1 : 0;
  }

  double * w    = (double *)malloc(sizeof(double) * D);
  double * grad = (double *)malloc(sizeof(double) * D);
  double * err  = (double *)malloc(sizeof(double) * N);  /* 各サンプルの誤差 (p - y) */
  for (int d = 0; d < D; d++) w[d] = 0.0;

  double loss = 0.0;
  long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0;
    correct = 0;
    /* 各サンプルの予測 p = sigmoid(w・x_i), 誤差 err[i] = p - y_i,
       損失・正解数を集計する。各サンプルは独立なので並列化できる。 */
    // BEGIN ANSWER: サンプルのループを #pragma omp parallel for reduction(+:loss,correct) で並列化せよ.
#pragma omp parallel for reduction(+:loss,correct)
    // END ANSWER
    for (long i = 0; i < N; i++) {
      double z = 0.0;
      for (int d = 0; d < D; d++) z += w[d] * X[i * D + d];
      double p = sigmoid(z);
      err[i] = p - (double)y[i];
      double eps = 1e-12;
      loss -= (y[i] ? log(p + eps) : log(1.0 - p + eps));
      int pred = (p > 0.5) ? 1 : 0;
      if (pred == y[i]) correct++;
    }
    loss /= (double)N;

    /* 勾配 grad[d] = (1/N) Σ_i err[i] * x_i[d]。特徴ごとに独立なので d で並列化 (競合なし)。 */
#pragma omp parallel for
    for (int d = 0; d < D; d++) {
      double g = 0.0;
      for (long i = 0; i < N; i++) g += err[i] * X[i * D + d];
      grad[d] = g / (double)N;
    }
    /* 重みの更新 */
    for (int d = 0; d < D; d++) w[d] -= lr * grad[d];

    if (ep % 50 == 0 || ep == E - 1)
      printf("epoch %3d: loss=%.4f, acc=%.2f%%\n", ep, loss, 100.0 * correct / N);
  }
  double elapsed = omp_get_wtime() - t0;

  printf("最終: N=%ld, D=%d, epochs=%d, loss=%.4f, acc=%.2f%%\n",
         N, D, E, loss, 100.0 * correct / N);
  printf("elapsed = %.3f sec\n", elapsed);
  free(w_true); free(X); free(y); free(w); free(grad); free(err);
  return 0;
}
