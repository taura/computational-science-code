#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

/* x = a*x + b を n 回繰り返す (2n flops). m 要素を独立に計算する. */
double lin_rec(double a, double b, double c, long n) {
  double x = c;
  for (long j = 0; j < n; j++) x = a * x + b;
  return x;
}

int main(int argc, char ** argv) {
  long m = (1 < argc ? atol(argv[1]) : 1024);
  long n = (2 < argc ? atol(argv[2]) : 1000 * 1000);
  double * x = (double *)calloc(sizeof(double), m);
  assert(x);

  double t0 = omp_get_wtime();
  /* このループは GPU 用にオフロード指示が付いている (完成済み).
     OMP_TARGET_OFFLOAD=DISABLED ならホスト(CPU)で, MANDATORY ならGPUで実行される. */
#pragma omp target teams distribute parallel for map(tofrom: x[0:m])
  for (long i = 0; i < m; i++) {
    x[i] = lin_rec(0.99, i + 1, 1.0, n);
  }
  double t1 = omp_get_wtime();
  double dt = t1 - t0;

  double flops = 2.0 * (double)m * (double)n;
  printf("m = %ld, n = %ld, elapsed = %.3f sec, %.3f GFLOPS\n",
         m, n, dt, flops / dt * 1e-9);
  free(x);
  return 0;
}
