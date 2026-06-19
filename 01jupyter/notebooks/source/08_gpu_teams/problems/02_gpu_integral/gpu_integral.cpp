#include <cstdio>
#include <cstdlib>
#include <cmath>

int main(int argc, char ** argv) {
  long n = (argc > 1 ? atol(argv[1]) : 100000000L);
  double dx = 1.0 / (double)n;
  double s = 0.0;

  /* 中点則で ∫_0^1 4/(1+x^2) dx = π を GPU 上で計算する.
     s は総和 (スカラ) なので reduction(+:s) を使う.
     スカラはコンパイラが自動的に転送するので map は不要. */
  // TODO: GPU上で reduction(+:s) を使って総和を求め π を計算せよ.

  double pi = s * dx;
  printf("n = %ld\n", n);
  printf("pi  = %.15f\n", pi);
  printf("M_PI = %.15f\n", M_PI);
  printf("error = %.3e\n", fabs(pi - M_PI));
  return 0;
}
