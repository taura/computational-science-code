#include <cstdio>

/* x を n 乗して返す関数 (x^n = x を n 回掛けたもの) */
double power(double x, int n) {
  double p = 1.0;
  // TODO: x を n 回掛けて x^n を計算し p に求めよ (ループを書く).
  // BEGIN ANSWER
  for (int i = 0; i < n; i++) {
    p = p * x;
  }
  // END ANSWER
  return p;
}

int main() {
  printf("2^10 = %f\n", power(2.0, 10));
  printf("3^4  = %f\n", power(3.0, 4));
  return 0;
}
