#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* 2D 拡散方程式 u_t = D (u_xx + u_yy) を陽解法 (FTCS) で時間発展させる。
   中央に置いたインクの塊が時間とともに広がる様子を計算する。
   更新式 (5点ラプラシアン, alpha = D*dt/dx^2, 安定条件 alpha <= 0.25):
     u^{n+1}[i][j] = u + alpha * (上 + 下 + 左 + 右 - 4*中央)
   境界は「反射 (断熱)」: 領域外の隣は自分自身とみなす (端の添字をはみ出さないよう留める)。
   → インクが外へ漏れないので, 全体の総量は時間によらず保存される (検算に使う)。 */
int main(int argc, char ** argv) {
  int    L     = (argc > 1 ? atoi(argv[1]) : 256);     /* 一辺の格子点数 */
  int    steps = (argc > 2 ? atoi(argv[2]) : 500);     /* 時間ステップ数 */
  double alpha = 0.2;                                  /* D*dt/dx^2, 安定 */

  int n = L * L;
  double * u  = (double *)calloc(n, sizeof(double));
  double * un = (double *)calloc(n, sizeof(double));
  /* 初期条件: 中央の正方形ブロックに濃度 1 のインクを置く。 */
  int lo = L / 2 - L / 16, hi = L / 2 + L / 16;
  for (int i = lo; i < hi; i++)
    for (int j = lo; j < hi; j++)
      u[i * L + j] = 1.0;
  double mass0 = 0.0;
  for (int k = 0; k < n; k++) mass0 += u[k];

  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    /* 全格子点を更新 (時間1ステップ進める)。端では添字を留めて反射境界にする。 */
    // TODO: 更新の二重ループを #pragma omp parallel for collapse(2) で並列化せよ.
    for (int i = 0; i < L; i++) {
      for (int j = 0; j < L; j++) {
        int im = (i > 0 ? i - 1 : i), ip = (i < L - 1 ? i + 1 : i);
        int jm = (j > 0 ? j - 1 : j), jp = (j < L - 1 ? j + 1 : j);
        double c = u[i * L + j];
        un[i * L + j] = c + alpha * (u[im * L + j] + u[ip * L + j]
                                   + u[i * L + jm] + u[i * L + jp] - 4.0 * c);
      }
    }
    double * tmp = u; u = un; un = tmp;
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算: 総量 (sum) が保存されているか。最大濃度は広がるほど下がる。 */
  double mass1 = 0.0, maxc = 0.0;
  for (int k = 0; k < n; k++) { mass1 += u[k]; if (u[k] > maxc) maxc = u[k]; }
  printf("L=%d, steps=%d: 総量 %.6f -> %.6f (保存されるはず), 最大濃度 %.6f\n",
         L, steps, mass0, mass1, maxc);
  printf("elapsed = %.3f sec\n", elapsed);
  free(u); free(un);
  return 0;
}
