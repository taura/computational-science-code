#include <cstdio>
#include <cstdlib>

enum { nl = 8 };   /* double 8つ = 512 bit のベクトル長 */
typedef double doublev __attribute__((vector_size(sizeof(double) * nl)));

/* 多項式 p(x) = c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 */
enum { deg = 3 };
static const double c[deg + 1] = { 1.0, 2.0, 3.0, 4.0 };

/* スカラ版 (検算用) */
static double poly_ref(double x) {
  double acc = c[deg];
  for (int k = deg - 1; k >= 0; k--) acc = acc * x + c[k];
  return acc;
}

int main(int argc, char ** argv) {
  /* n は簡単のため nl の倍数とする */
  long n = (argc > 1 ? atol(argv[1]) : 64);
  double * x = (double *)malloc(sizeof(double) * n);
  double * p = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) x[i] = 0.001 * (double)i;

  /* nl 個ずつまとめて Horner 法 acc = acc*x + c_k で多項式を評価する */
  for (long i = 0; i < n; i += nl) {
    doublev xv = *(doublev *)&x[i];
    doublev acc;
    // TODO: Horner法 acc = acc*x + c_k をベクトルのまま計算して多項式を評価せよ.
    *(doublev *)&p[i] = acc;
  }

  long err = 0;
  for (long i = 0; i < n; i++) {
    double r = poly_ref(x[i]);
    double d = p[i] - r;
    if (d < -1e-9 || d > 1e-9) err++;
  }
  if (err == 0) printf("OK: p[0]=%.3f, p[%ld]=%.3f\n", p[0], n - 1, p[n - 1]);
  else          printf("NG: %ld 要素が不正\n", err);
  free(x); free(p);
  return 0;
}
