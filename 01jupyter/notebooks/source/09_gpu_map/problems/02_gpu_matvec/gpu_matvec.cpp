#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 4096);
  double * A = (double *)malloc(sizeof(double) * n * n);
  double * x = (double *)malloc(sizeof(double) * n);
  double * y = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) {
    x[i] = 1.0;
    y[i] = -1.0;            /* 番兵: 未計算なら検算に失敗する */
    for (long j = 0; j < n; j++) A[i * n + j] = 1.0;
  }

  /* 行列ベクトル積 y = A x を GPU で計算する.
     A は n*n 要素, x, y は n 要素. A,x は入力 (to:), y は結果 (from:). */
  // TODO: 行列ベクトル積をGPUにオフロードし, A,x は map(to:), 結果 y は map(from:) で受け取れ.

  /* 検算: A[i][j]=1, x[j]=1 なので y[i] = n になるはず */
  long err = 0;
  for (long i = 0; i < n; i++) {
    if (y[i] != (double)n) err++;
  }
  if (err == 0) {
    printf("OK: n = %ld, y[0] = %.0f (= n)\n", n, y[0]);
  } else {
    printf("NG: %ld 要素が不正 (例: y[0] = %.0f, 正解は %ld)\n", err, y[0], n);
  }
  free(A); free(x); free(y);
  return 0;
}
