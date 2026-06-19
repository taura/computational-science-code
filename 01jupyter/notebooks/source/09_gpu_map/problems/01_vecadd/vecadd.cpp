#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 1000);
  double * a = (double *)malloc(sizeof(double) * n);
  double * b = (double *)malloc(sizeof(double) * n);
  double * c = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) { a[i] = i; b[i] = 2 * i; c[i] = -1.0; }

  /* c[i] = a[i] + b[i] を GPU で計算する */
  // TODO: ループをGPUにオフロードして c[i]=a[i]+b[i] を計算せよ. a,b は map(to:), 結果 c は map(from:) で受け取る.

  /* 検算 */
  long err = 0;
  for (long i = 0; i < n; i++) {
    if (c[i] != a[i] + b[i]) err++;
  }
  if (err == 0) {
    printf("OK: c[0] = %.0f, c[%ld] = %.0f\n", c[0], n - 1, c[n - 1]);
  } else {
    printf("NG: %ld 要素が不正 (例: c[0] = %.0f, 正解は %.0f)\n", err, c[0], a[0] + b[0]);
  }
  free(a); free(b); free(c);
  return 0;
}
