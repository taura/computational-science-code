#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <omp.h>

/* x = ax + b をひたすら n 回繰り返す.
   (|a| < 1.0 なら c によらず x = b / (1 - a) に収束).
   1 回につき乗算 1 回・加算 1 回, 計 2n flops を行う */
double lin_rec(double a, double b, double c, long n) {
  double t = c;
  for (long j = 0; j < n; j++) {
    t = a * t + b;
  }
  return t;
}

int main(int argc, char ** argv) {
  long m     = (1 < argc ? atol(argv[1]) : 8);
  long n     = (2 < argc ? atol(argv[2]) : 1000 * 1000 * 1000);
  double * x = (double *)calloc(sizeof(double), m);
  assert(x);
  printf("m = %ld, n = %ld\n", m, n);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体 */
  for (long i = 0; i < m; i++) {
    x[i] = lin_rec(0.99, i + 1, 1.0, n);
  }
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */

  /* 答え表示 (x[i] = 100 * (i + 1) くらいのはず) */
  long err = 0;
  for (long i = 0; i < m; i++) {
    if (fabs(x[i] - 100 * (i + 1)) > 1.0e-3) {
      printf("x[%3ld] = %9.3f\n", i, x[i]);
      err++;
    }
  }
  if (err == 0) {
    printf("OK\n");
  }
  double flops = 2. * (double)m * (double)n;
  printf("elapsed    : %7.3f  sec\n", dt);
  printf("elapsed/m  : %7.3f msec\n", dt / m * 1e3);
  printf("elapsed/n  : %7.3f nsec\n", dt / n * 1e9);
  printf("elapsed/mn : %7.3f nsec\n", dt / (m * n) * 1e9);
  printf("flops      : %.2e\n", flops);
  printf("%.3f GFLOPS\n", flops / dt * 1e-9);
  return 0;
}
