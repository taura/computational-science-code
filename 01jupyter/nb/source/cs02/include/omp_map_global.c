#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
typedef struct { float x; float y; } point;
float t;
float a[3];
point p;
int main(int argc, char ** argv) {
  int i = 1;
  t = (argc > i ? atof(argv[i]) : 10.0); i++;
  for (int i = 0; i < 3; i++) { a[i] = t + i; }
  p.x = t + 3; p.y = t + 4;
/*** if VER >= 2 */
  printf("[host] t @ %p = %f\n", &t, t);
  printf("[host] a @ %p = { %f, %f, %f }\n", a, a[0], a[1], a[2]);
  printf("[host] p @ %p = { %f, %f }\n", &p, p.x, p.y);
/*** endif */
#pragma omp target
  {
/*** if VER == 1 */
    printf("GPU: t = %f\n", t);
    printf("GPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
    printf("GPU: p = { %f, %f }\n", p.x, p.y);
    t *= 2.0;
    for (int i = 0; i < 3; i++) a[i] *= 2.0;
    p.x *= 2.0; p.y *= 2.0;
/*** else */
    printf("[dev ] t @ %p = %f\n", &t, t);
    printf("[dev ] a @ %p = { %f, %f, %f }\n", a, a[0], a[1], a[2]);
    printf("[dev ] p @ %p = { %f, %f }\n", &p, p.x, p.y);
/*** endif */
  }
  printf("CPU: t = %f\n", t);
  printf("CPU: a = { %f, %f, %f }\n", a[0], a[1], a[2]);
  printf("CPU: p = { %f, %f }\n", p.x, p.y);
  return 0;
}
