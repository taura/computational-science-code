#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  /* 要素数を実行時に決める(動的な)配列 */
  long n = (argc > 1 ? atol(argv[1]) : 5);
  /* malloc で n 要素分の領域を確保する (calloc は 0 初期化付き) */
  double * a = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) {
    a[i] = 1.0 / (i + 1);     /* 1/1, 1/2, 1/3, ... */
  }
  double s = 0.0;
  for (long i = 0; i < n; i++) {
    s += a[i];
  }
  printf("sum of 1/k (k=1..%ld) = %f\n", n, s);
  free(a);                    /* 使い終わったら解放する */
  return 0;
}
