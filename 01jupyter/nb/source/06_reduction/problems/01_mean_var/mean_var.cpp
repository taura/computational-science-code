#include <cstdio>
#include <cmath>

int main() {
  const long n = 1000000L;
  static double a[n];
  for (long i = 0; i < n; i++) {
    a[i] = sin((double)i);
  }
  double s = 0.0, sq = 0.0;
  // TODO: 下のループを #pragma omp parallel for reduction(+:s,sq) で並列化し, 2つの総和の競合を解消せよ.
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:s,sq)
  // END ANSWER
  for (long i = 0; i < n; i++) {
    double x = a[i];
    s  += x;
    sq += x * x;
  }
  double mean = s / n;
  double var  = sq / n - mean * mean;
  printf("mean = %f, variance = %f\n", mean, var);
  return 0;
}
