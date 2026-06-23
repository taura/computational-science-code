#include <cstdio>
#include <cmath>

int main() {
  const long n = 400000000L;
  double s = 0.0, sq = 0.0;
  // TODO: 下のループを #pragma omp parallel for reduction(+:s,sq) で並列化し, 2つの総和の競合を解消せよ.
  // BEGIN ANSWER
#pragma omp parallel for reduction(+:s,sq)
  // END ANSWER
  for (long i = 0; i < n; i++) {
    double x = sin((double)(i % 1000));   /* データを配列に置かず逐次生成 (並列化の本質は2つの総和) */
    s  += x;
    sq += x * x;
  }
  double mean = s / n;
  double var  = sq / n - mean * mean;
  printf("mean = %f, variance = %f\n", mean, var);
  return 0;
}
