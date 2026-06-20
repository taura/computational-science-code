#include <cstdio>
#include <cstdlib>
#include <cmath>

/* 2D 定常熱伝導 (ラプラス方程式) をヤコビ反復で解く。
   L×L の板。上端(行0)の温度を 100, 残り3辺を 0 に固定し, 内部が定常分布に
   落ち着くまで反復する。各内部点を上下左右4点の平均で更新する (5点ステンシル)。 */
int main(int argc, char ** argv) {
  int    L       = (argc > 1 ? atoi(argv[1]) : 129);     /* 一辺の格子点数 */
  double tol     = (argc > 2 ? atof(argv[2]) : 1e-6);    /* 収束判定 (最大更新量) */
  int    maxiter = (argc > 3 ? atoi(argv[3]) : 1000000);

  int n = L * L;
  double * u    = (double *)malloc(sizeof(double) * n);
  double * unew = (double *)malloc(sizeof(double) * n);
  /* 初期化: 上端(行0)=100, 他=0。境界はずっと固定なので両配列に同じ値を入れておく。 */
  for (int i = 0; i < L; i++)
    for (int j = 0; j < L; j++) {
      double val = (i == 0) ? 100.0 : 0.0;
      u[i * L + j] = val;
      unew[i * L + j] = val;
    }

  int iter;
  double diff = 0.0;
  for (iter = 0; iter < maxiter; iter++) {
    diff = 0.0;   /* この反復での最大更新量 */
    /* 内部の各点 (i,j) を上下左右の平均で更新し, 更新量の最大値を求める。 */
    // BEGIN ANSWER: 内側の二重ループを #pragma omp parallel for collapse(2) reduction(max:diff) で並列化せよ.
#pragma omp parallel for collapse(2) reduction(max:diff)
    // END ANSWER
    for (int i = 1; i < L - 1; i++) {
      for (int j = 1; j < L - 1; j++) {
        double v = 0.25 * (u[(i - 1) * L + j] + u[(i + 1) * L + j]
                         + u[i * L + (j - 1)] + u[i * L + (j + 1)]);
        double d = fabs(v - u[i * L + j]);
        if (d > diff) diff = d;
        unew[i * L + j] = v;
      }
    }
    /* u と unew を入れ替える (コピーせずポインタを差し替えるだけ) */
    double * tmp = u; u = unew; unew = tmp;
    if (diff < tol) break;
  }

  printf("L=%d, iters=%d, 最終残差=%.2e, 中心温度=%.4f (理論 25.0)\n",
         L, iter + 1, diff, u[(L / 2) * L + (L / 2)]);
  free(u); free(unew);
  return 0;
}
