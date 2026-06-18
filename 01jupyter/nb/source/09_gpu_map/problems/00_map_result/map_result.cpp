#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char ** argv) {
  float t = (argc > 1 ? atof(argv[1]) : 10.0);
  float a[3] = { t + 1, t + 2, t + 3 };
  // BEGIN ANSWER: GPUで更新した結果がホストに反映されるよう, target 構文に map(tofrom: ...) を付けよ.
#pragma omp target map(tofrom: t, a[0:3])
  // END ANSWER
  {
    printf("GPU: t = %f\n", t);
    printf("GPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
    t *= 2.0;
    for (int i = 0; i < 3; i++) a[i] *= 2.0;
  }
  printf("CPU: t = %f\n", t);
  printf("CPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
  return 0;
}
