#include <cstdio>

int main() {
  int    i = 42;            /* 整数 (普通32 bit) */
  long   l = 10000000000L;  /* 長い整数 (普通64 bit) */
  float  f = 3.14f;         /* 単精度浮動小数点 (32 bit) */
  double d = 3.141592653589793;  /* 倍精度浮動小数点 (64 bit) */

  printf("i = %d\n", i);
  printf("l = %ld\n", l);
  printf("f = %f\n", f);
  printf("d = %.15f\n", d);

  /* それぞれが何バイトかを表示する */
  printf("sizeof(int)    = %zu bytes\n", sizeof(int));
  printf("sizeof(long)   = %zu bytes\n", sizeof(long));
  printf("sizeof(float)  = %zu bytes\n", sizeof(float));
  printf("sizeof(double) = %zu bytes\n", sizeof(double));

  /* 整数の割り算は切り捨て, 浮動小数点の割り算は普通の割り算 */
  printf("7 / 2     = %d\n", 7 / 2);
  printf("7.0 / 2.0 = %f\n", 7.0 / 2.0);
  return 0;
}
