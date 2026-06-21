#include <cstdio>

int main() {
  double c = 36.5;   /* 摂氏 */
  double f;          /* 華氏 (これを求める) */
  // TODO: 摂氏 c を華氏 f に変換する式を書け (f = c * 9/5 + 32). 整数の割り算に注意.
  // BEGIN ANSWER
  f = c * 9.0 / 5.0 + 32.0;
  // END ANSWER
  printf("%.1f C = %.1f F\n", c, f);
  return 0;
}
