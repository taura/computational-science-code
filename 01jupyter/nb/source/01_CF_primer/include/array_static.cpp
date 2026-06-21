#include <cstdio>

int main() {
  /* 要素数を固定した(静的な)配列. 添字は 0 から n-1 */
  const int n = 5;
  double a[n];
  for (int i = 0; i < n; i++) {
    a[i] = i * i;        /* a[i] = i の二乗 */
  }
  double s = 0.0;
  for (int i = 0; i < n; i++) {
    printf("a[%d] = %f\n", i, a[i]);
    s += a[i];
  }
  printf("sum = %f\n", s);
  return 0;
}
