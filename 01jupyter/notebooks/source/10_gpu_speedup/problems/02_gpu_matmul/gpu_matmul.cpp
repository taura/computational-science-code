#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <omp.h>

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 1024);
  double * A = (double *)malloc(sizeof(double) * n * n);
  double * B = (double *)malloc(sizeof(double) * n * n);
  double * C = (double *)malloc(sizeof(double) * n * n);
  for (long i = 0; i < n * n; i++) { A[i] = 1.0; B[i] = 1.0; C[i] = 0.0; }

  double t0 = omp_get_wtime();
  /* 行列積 C = A * B (完成済み).
     OMP_TARGET_OFFLOAD=DISABLED ならホスト(CPU)で, MANDATORY ならGPUで実行される. */
#pragma omp target teams distribute parallel for map(to: A[0:n*n], B[0:n*n]) map(from: C[0:n*n])
  for (long i = 0; i < n; i++) {
    for (long j = 0; j < n; j++) {
      double s = 0.0;
      for (long k = 0; k < n; k++) s += A[i * n + k] * B[k * n + j];
      C[i * n + j] = s;
    }
  }
  double t1 = omp_get_wtime();
  double dt = t1 - t0;

  /* 検算: A,B が全て 1 なので C[i][j] = n になるはず */
  long err = 0;
  for (long i = 0; i < n * n; i++) if (C[i] != (double)n) err++;

  double flops = 2.0 * (double)n * (double)n * (double)n;
  printf("n = %ld, elapsed = %.3f sec, %.3f GFLOPS, %s\n",
         n, dt, flops / dt * 1e-9, (err == 0 ? "OK" : "NG"));
  free(A); free(B); free(C);
  return 0;
}
