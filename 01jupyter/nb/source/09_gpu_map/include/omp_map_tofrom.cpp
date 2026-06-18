#include <cstdio>
#include <cstdlib>
#include <omp.h>

typedef struct { float x; float y; } point;

int main(int argc, char ** argv) {
  float t = (argc > 1 ? atof(argv[1]) : 10.0);
  float a[3] = { t, t + 1, t + 2 };
  point p = { t + 3, t + 4 };
  // map(tofrom: ...) を指定すると, GPUでの書き換えがCPUに戻ってくる
  // 配列は a[0:3] のように「先頭:要素数」で範囲を指定する
#pragma omp target map(tofrom: t, a[0:3], p)
  {
    printf("GPU: t = %f\n", t);
    printf("GPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
    printf("GPU: p = { %f, %f }\n", p.x, p.y);
    t *= 2.0;
    for (int i = 0; i < 3; i++) a[i] *= 2.0;
    p.x *= 2.0; p.y *= 2.0;
  }
  printf("CPU: t = %f\n", t);
  printf("CPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
  printf("CPU: p = { %f, %f }\n", p.x, p.y);
  return 0;
}
