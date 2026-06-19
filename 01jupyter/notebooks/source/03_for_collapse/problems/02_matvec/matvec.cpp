#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  // 行列・ベクトルのサイズ (コマンドライン引数, 既定 4000)
  int n = (argc > 1) ? atoi(argv[1]) : 4000;

  double *A = (double *)malloc((size_t)n * n * sizeof(double));
  double *x = (double *)malloc((size_t)n * sizeof(double));
  double *y = (double *)malloc((size_t)n * sizeof(double));

  // 検算しやすい初期化: A[i*n+j] = 1, x[j] = 1 とすると y[i] = n になる.
  // (この初期化ループは collapse(2) で並列化してもよい)
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      A[(size_t)i * n + j] = 1.0;
    }
  }
  for (int j = 0; j < n; j++) {
    x[j] = 1.0;
  }

  // 行列ベクトル積 y = A x
  // TODO: 下の行 (外側の i ループ) の直前に #pragma omp parallel for を1行追加し, 行ごとの計算をスレッドで分担せよ.
  for (int i = 0; i < n; i++) {
    double s = 0.0;  // 行ごとの局所アキュムレータ (reduction 不要)
    for (int j = 0; j < n; j++) {
      s += A[(size_t)i * n + j] * x[j];
    }
    y[i] = s;
  }

  // 検算: すべての y[i] が n に等しいはず
  int ok = 1;
  for (int i = 0; i < n; i++) {
    if (y[i] != (double)n) {
      ok = 0;
      break;
    }
  }
  printf("n = %d, y[0] = %f (expected %d): %s\n",
         n, y[0], n, ok ? "OK" : "NG");

  free(A);
  free(x);
  free(y);
  return 0;
}
