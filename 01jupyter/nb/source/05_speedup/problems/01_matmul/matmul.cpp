#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <omp.h>

/* 密行列積 C = A * B (いずれも n x n).
   行列は 1 次元配列に格納する (A[i*n+j] が A の (i,j) 要素).
   浮動小数点演算は乗算 + 加算が n 回 / 要素なので, 全体で 2 n^3 flops. */

int main(int argc, char ** argv) {
  long n = (1 < argc ? atol(argv[1]) : 1000);
  double * A = (double *)malloc(sizeof(double) * n * n);
  double * B = (double *)malloc(sizeof(double) * n * n);
  double * C = (double *)malloc(sizeof(double) * n * n);
  assert(A && B && C);
  /* A も B も全要素 1.0 に初期化 -> C[i][j] = n になるはず (検算しやすい) */
  for (long i = 0; i < n * n; i++) { A[i] = 1.0; B[i] = 1.0; C[i] = 0.0; }
  printf("n = %ld\n", n);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体: 3 重ループ. C[i][j] += A[i][k] * B[k][j] */
  // BEGIN ANSWER: いちばん外側の i ループの直前に #pragma omp parallel for を1行追加し, 行ごとに並列化せよ.
#pragma omp parallel for
  // END ANSWER
  for (long i = 0; i < n; i++) {
    for (long j = 0; j < n; j++) {
      double s = 0.0;
      for (long k = 0; k < n; k++) {
        s += A[i * n + k] * B[k * n + j];
      }
      C[i * n + j] = s;
    }
  }
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */
  /* 検算: 全要素が n のはずなので, 総和は n * n * n */
  double checksum = 0.0;
  for (long i = 0; i < n * n; i++) checksum += C[i];
  double expected = (double)n * (double)n * (double)n;
  printf("checksum   : %.0f (expected %.0f) -> %s\n",
         checksum, expected, (checksum == expected ? "OK" : "NG"));
  double flops = 2. * (double)n * (double)n * (double)n;
  printf("elapsed    : %7.3f  sec\n", dt);
  printf("flops      : %.2e\n", flops);
  printf("%.3f GFLOPS\n", flops / dt * 1e-9);
  return 0;
}
