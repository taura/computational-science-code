#include <cstdio>

int main() {
  const int n = 10;
  double a[n];
  /* 配列を a[i] = (i+1) の二乗 で埋める */
  for (int i = 0; i < n; i++) {
    a[i] = (i + 1) * (i + 1);
  }
  /* 合計を求める */
  double s = 0.0;
  // BEGIN ANSWER: 配列 a の全要素を s に足し込むループを書け.
  for (int i = 0; i < n; i++) {
    s += a[i];
  }
  // END ANSWER
  printf("sum of squares 1..%d = %.0f\n", n, s);
  return 0;
}
