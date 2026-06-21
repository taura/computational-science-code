#include <cstdio>

/* 2数の平均を返す関数 (引数2つ, 返り値1つ) */
double average(double a, double b) {
  return (a + b) / 2.0;
}

/* n! を計算する関数 */
long factorial(int n) {
  long p = 1;
  for (int i = 2; i <= n; i++) {
    p = p * i;
  }
  return p;
}

int main() {
  printf("average(3.0, 4.0) = %f\n", average(3.0, 4.0));
  printf("factorial(5)      = %ld\n", factorial(5));
  return 0;
}
