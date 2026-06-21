#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  /* 要素数 n を実行時 (コマンドライン引数) で決める */
  long n = (argc > 1 ? atol(argv[1]) : 100);
  double * a = NULL;
  // TODO: a に double n 個分の領域を malloc で確保せよ.
  // BEGIN ANSWER
  a = (double *)malloc(sizeof(double) * n);
  // END ANSWER
  for (long i = 0; i < n; i++) {
    a[i] = 1.0 / (i + 1);   /* 1/1, 1/2, 1/3, ... */
  }
  double s = 0.0;
  for (long i = 0; i < n; i++) {
    s += a[i];
  }
  printf("sum of 1/k (k=1..%ld) = %f\n", n, s);
  free(a);                  /* 確保した領域は解放する */
  return 0;
}
