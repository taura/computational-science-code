#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 状態を持たない乱数 (合成データ・初期値生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

static inline double sigmoidf(double z) { return 1.0 / (1.0 + exp(-z)); }

/* 多層パーセプトロン (MLP) を自分で学習させる。
   ネットワーク: 入力 2 -> 隠れ層 H (tanh) -> 出力 1 (sigmoid)。
   2次元データの二値分類で, クラス境界が「円」(非線形分離) なので
   隠れ層が必須。AI の「学習」の中身が行列積であることを体験する。

   forward:  h_k = tanh( Σ_d W1[k,d] x_d + b1[k] ),  o = sigmoid( Σ_k W2[k] h_k + b2 )
   損失:     二値クロスエントロピー
   backprop: do = o - y,  dW2[k] += do * h_k,  db2 += do,
             dh_k = do * W2[k] * (1 - h_k^2),
             dW1[k,d] += dh_k * x_d,  db1[k] += dh_k
   更新:     全サンプルにわたる勾配の和を取り, W -= lr * grad/N。
   並列化対象は「全サンプルにわたる勾配の和」(配列 reduction)。 */
int main(int argc, char ** argv) {
  long N = (argc > 1 ? atol(argv[1]) : 4000);   /* サンプル数 */
  int  H = (argc > 2 ? atoi(argv[2]) : 32);     /* 隠れ層のユニット数 */
  int  E = (argc > 3 ? atoi(argv[3]) : 3000);   /* エポック数 */
  double lr = (argc > 4 ? atof(argv[4]) : 0.7); /* 学習率 */
  const int D = 2;                              /* 入力次元 */
  const double R2 = 0.5;                        /* 内側の円の半径^2 */

  /* 合成データ: [-1,1]^2 上の点。原点に近い (内側の円) なら class 1。非線形分離。 */
  double * X = (double *)malloc(sizeof(double) * N * D);
  int    * y = (int *)malloc(sizeof(int) * N);
  for (long i = 0; i < N; i++) {
    double x0 = draw_rand01(i, 0) * 2.0 - 1.0;
    double x1 = draw_rand01(i, 1) * 2.0 - 1.0;
    X[i*D+0] = x0; X[i*D+1] = x1;
    y[i] = (x0*x0 + x1*x1 < R2) ? 1 : 0;
  }

  /* パラメータ: W1[H][D], b1[H], W2[H], b2。小さな乱数で初期化。 */
  double * W1 = (double *)malloc(sizeof(double) * H * D);
  double * b1 = (double *)malloc(sizeof(double) * H);
  double * W2 = (double *)malloc(sizeof(double) * H);
  double   b2 = 0.0;
  for (int k = 0; k < H; k++) {
    for (int d = 0; d < D; d++) W1[k*D+d] = (draw_rand01(k, d+10) - 0.5);
    b1[k] = 0.0;
    W2[k] = (draw_rand01(k, 99) - 0.5);
  }

  /* 勾配の総和を入れる配列 */
  double * gW1 = (double *)malloc(sizeof(double) * H * D);
  double * gb1 = (double *)malloc(sizeof(double) * H);
  double * gW2 = (double *)malloc(sizeof(double) * H);

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    for (int k = 0; k < H; k++) { gW1[k*D+0]=0; gW1[k*D+1]=0; gb1[k]=0; gW2[k]=0; }
    double gb2 = 0.0;
    loss = 0.0; correct = 0;

    /* 全サンプルにわたる forward + backprop。各サンプルの勾配寄与を総和する。
       損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。 */
    // TODO: サンプルのループを配列 reduction で並列化せよ: #pragma omp parallel for reduction(+:loss,correct,gb2,gW1[:H*D],gb1[:H],gW2[:H]).
    for (long i = 0; i < N; i++) {
      double x0 = X[i*D+0], x1 = X[i*D+1];
      /* forward */
      double o_in = b2;
      double h[256];                     /* H <= 256 を仮定 */
      for (int k = 0; k < H; k++) {
        double z = b1[k] + W1[k*D+0]*x0 + W1[k*D+1]*x1;
        double hk = tanh(z);
        h[k] = hk;
        o_in += W2[k] * hk;
      }
      double o = sigmoidf(o_in);
      double yi = (double)y[i];
      double eps = 1e-12;
      loss -= (y[i] ? log(o + eps) : log(1.0 - o + eps));
      if (((o > 0.5) ? 1 : 0) == y[i]) correct++;
      /* backprop */
      double dout = o - yi;
      gb2 += dout;
      for (int k = 0; k < H; k++) {
        gW2[k] += dout * h[k];
        double dh = dout * W2[k] * (1.0 - h[k]*h[k]);
        gW1[k*D+0] += dh * x0;
        gW1[k*D+1] += dh * x1;
        gb1[k]     += dh;
      }
    }
    loss /= (double)N;

    /* 更新 (勾配を平均して降下) */
    double s = lr / (double)N;
    for (int k = 0; k < H; k++) {
      W1[k*D+0] -= s * gW1[k*D+0];
      W1[k*D+1] -= s * gW1[k*D+1];
      b1[k]     -= s * gb1[k];
      W2[k]     -= s * gW2[k];
    }
    b2 -= s * gb2;

    if (ep % 500 == 0 || ep == E - 1)
      printf("epoch %4d: loss=%.4f, acc=%.2f%%\n", ep, loss, 100.0 * correct / N);
  }
  double elapsed = omp_get_wtime() - t0;

  printf("最終: N=%ld, H=%d, epochs=%d, loss=%.4f, acc=%.2f%%\n",
         N, H, E, loss, 100.0 * correct / N);
  printf("elapsed = %.3f sec\n", elapsed);
  free(X); free(y); free(W1); free(b1); free(W2); free(gW1); free(gb1); free(gW2);
  return 0;
}
