#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  /* 第1引数を整数 n, 第2引数を実数 x として受け取り, x の n 乗を表示する.
     引数が無いときの既定値は n=3, x=2.0 */
  int n = 3;
  double x = 2.0;
  // TODO: argv[1] を atoi で n に, argv[2] を atof で x に変換せよ (引数があるときだけ).
  double p = 1.0;
  for (int i = 0; i < n; i++) p *= x;
  printf("%f ^ %d = %f\n", x, n, p);
  return 0;
}
