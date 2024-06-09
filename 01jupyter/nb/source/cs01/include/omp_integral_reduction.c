#include <stdio.h>
#include <stdlib.h>

double int_inv_1_x2(double a, double b, long n) {
  double s = 0.0;
  double dx = (b - a) / (double)n;
#pragma omp parallel for reduction(+:s)
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
