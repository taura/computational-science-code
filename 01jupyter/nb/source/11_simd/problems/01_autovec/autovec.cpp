#include <cstdio>
#include <cstdlib>

/* saxpy: y[i] = a*x[i] + y[i].
   x と y がポインタなので, コンパイラは「x と y が重なって (エイリアスして) いるかも
   しれない」と考えざるを得ない. そのため自動ベクトル化はできても, 実行時に重なりを
   確認して2通りのコードに分岐する (versioning) ことがある. */
void saxpy(long n, double a, double * x, double * y) {
  for (long i = 0; i < n; i++) {
    y[i] = a * x[i] + y[i];
  }
}

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 16);
  double * x = (double *)malloc(sizeof(double) * n);
  double * y = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) { x[i] = i; y[i] = 0.0; }
  saxpy(n, 2.0, x, y);
  printf("y[0] = %.1f, y[%ld] = %.1f\n", y[0], n - 1, y[n - 1]);
  free(x); free(y);
  return 0;
}
