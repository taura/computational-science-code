#include <cstdio>
#include <cstdlib>

/* 内積 s = Σ x[i]*y[i] を n 要素について計算する */
double dot(long n, double * x, double * y) {
  double s = 0.0;
  // TODO: 内積の総和ループを simd reduction でSIMD化せよ (下の for の直前に1行追加).
  for (long i = 0; i < n; i++) {
    s += x[i] * y[i];
  }
  return s;
}

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 100L * 1000 * 1000);
  double * x = (double *)malloc(sizeof(double) * n);
  double * y = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) { x[i] = 1.0; y[i] = 2.0; }

  double s = dot(n, x, y);

  /* x[i]=1, y[i]=2 なので理論値は 2*n */
  double expected = 2.0 * (double)n;
  if (s == expected) printf("OK: s=%.1f (= 2*n)\n", s);
  else               printf("NG: s=%.1f, expected=%.1f\n", s, expected);
  free(x); free(y);
  return 0;
}
