#include <cstdio>
#include <cstdlib>

double int_inv_1_x2(double a, double b, long n) {
  double s = 0.0;
  double dx = (b - a) / (double)n;
  // BEGIN ANSWER: 下のループを reduction(+:s) を用いて並列化し, 総和の競合を解消せよ.
#pragma omp parallel for reduction(+:s)
  // END ANSWER
  for (long i = 0; i < n; i++) {
    double x = a + i * dx;
    s += 1 / (1 + x * x);
  }
  return s * dx;
}

int main(int argc, char ** argv) {
  double a = (argc > 1 ? atof(argv[1]) : 0.0);
  double b = (argc > 2 ? atof(argv[2]) : 1.0);
  long n   = (argc > 3 ? atol(argv[3]) : 1000L * 1000L * 1000L);
  double s = int_inv_1_x2(a, b, n);
  printf("s = %f\n", s);
  return 0;
}
