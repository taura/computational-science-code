#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* C = A * B (いずれも n x n 行列, 行優先). i-k-j 順のループ. */
void matmul(long n, double * A, double * B, double * C) {
  for (long i = 0; i < n; i++)
    for (long j = 0; j < n; j++) C[i * n + j] = 0.0;

  #pragma omp parallel for
  for (long i = 0; i < n; i++) {
    for (long k = 0; k < n; k++) {
      double a = A[i * n + k];
      // TODO: 最内 j ループを omp simd でSIMD化せよ (下の for の直前に1行追加).
      for (long j = 0; j < n; j++) {
        C[i * n + j] += a * B[k * n + j];
      }
    }
  }
}

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 1024);
  double * A = (double *)malloc(sizeof(double) * n * n);
  double * B = (double *)malloc(sizeof(double) * n * n);
  double * C = (double *)malloc(sizeof(double) * n * n);
  for (long i = 0; i < n * n; i++) { A[i] = 1.0; B[i] = 2.0; }

  double t0 = omp_get_wtime();
  matmul(n, A, B, C);
  double dt = omp_get_wtime() - t0;

  double gflops = 2.0 * (double)n * n * n / dt * 1e-9;
  /* A[i]=1, B[i]=2 なので C の各要素は 2*n になるはず. checksum で確認. */
  double expected = 2.0 * (double)n;
  long err = 0;
  for (long i = 0; i < n * n; i++) if (C[i] != expected) err++;
  printf("n=%ld : %.3f GFLOPS  (check: %s)\n", n, gflops, err == 0 ? "OK" : "NG");
  free(A); free(B); free(C);
  return 0;
}
