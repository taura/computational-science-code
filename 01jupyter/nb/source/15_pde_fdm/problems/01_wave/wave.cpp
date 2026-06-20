#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* 2D 波動方程式 u_tt = c^2 (u_xx + u_yy) を陽解法で時間発展させる。
   L×L の膜。四辺を固定 (u=0) し, 中央に山(ガウス)を置いて波が広がり反射する様子を計算する。
   更新式 (5点ラプラシアン, coef = (c*dt/dx)^2, 安定条件 coef <= 0.5):
     u^{n+1} = 2 u^n - u^{n-1} + coef * (上+下+左+右 - 4*中央)
   時間方向は前後のステップに依存するので逐次, 空間の二重ループを並列化する。 */
int main(int argc, char ** argv) {
  int    L     = (argc > 1 ? atoi(argv[1]) : 257);     /* 一辺の格子点数 */
  int    steps = (argc > 2 ? atoi(argv[2]) : 200);     /* 時間ステップ数 */
  double coef  = 0.25;                                 /* (c*dt/dx)^2, 安定 */

  int n = L * L;
  double * up = (double *)calloc(n, sizeof(double));   /* u^{n-1} */
  double * cu = (double *)calloc(n, sizeof(double));   /* u^{n}   */
  double * nx = (double *)calloc(n, sizeof(double));   /* u^{n+1} */
  /* 初期条件: 中央にガウスの山, 初速 0 (= up と cu を同じにする)。境界は 0 のまま。 */
  double c0 = (L - 1) / 2.0, sig = L / 16.0;
  for (int i = 1; i < L - 1; i++)
    for (int j = 1; j < L - 1; j++) {
      double r2 = (i - c0) * (i - c0) + (j - c0) * (j - c0);
      double v = exp(-r2 / (2.0 * sig * sig));
      up[i * L + j] = v;
      cu[i * L + j] = v;
    }

  double t0 = omp_get_wtime();
  for (int t = 0; t < steps; t++) {
    /* 内部の各点を更新 (時間1ステップ進める) */
    // BEGIN ANSWER: 内側の二重ループを #pragma omp parallel for collapse(2) で並列化せよ.
#pragma omp parallel for collapse(2)
    // END ANSWER
    for (int i = 1; i < L - 1; i++) {
      for (int j = 1; j < L - 1; j++) {
        double lap = cu[(i - 1) * L + j] + cu[(i + 1) * L + j]
                   + cu[i * L + (j - 1)] + cu[i * L + (j + 1)]
                   - 4.0 * cu[i * L + j];
        nx[i * L + j] = 2.0 * cu[i * L + j] - up[i * L + j] + coef * lap;
      }
    }
    /* up <- cu <- nx と時間を1つ進める (ポインタを回す) */
    double * tmp = up; up = cu; cu = nx; nx = tmp;
  }
  double elapsed = omp_get_wtime() - t0;

  /* 検算1: 最大振幅 (発散していなければ O(1) のまま)。
     検算2: 初期条件が i<->j 対称なので, 解も常に対称。max|u[i][j]-u[j][i]| は丸め誤差程度。 */
  double maxabs = 0.0, asym = 0.0;
  for (int i = 0; i < L; i++)
    for (int j = 0; j < L; j++) {
      double a = fabs(cu[i * L + j]);
      if (a > maxabs) maxabs = a;
      double s = fabs(cu[i * L + j] - cu[j * L + i]);
      if (s > asym) asym = s;
    }
  printf("L=%d, steps=%d: 最大振幅=%.4f, 対称性誤差=%.2e (≈0 なら正しい)\n",
         L, steps, maxabs, asym);
  printf("elapsed = %.3f sec\n", elapsed);
  free(up); free(cu); free(nx);
  return 0;
}
