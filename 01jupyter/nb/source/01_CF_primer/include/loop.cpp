#include <cstdio>

int main() {
  /* for 文: 初期化; 条件; 更新 */
  printf("for loop:\n");
  for (int i = 0; i < 5; i++) {
    printf("  i = %d\n", i);
  }

  /* while 文: 条件が成り立つ間繰り返す */
  printf("while loop:\n");
  int j = 1;
  long p = 1;
  while (p < 100) {
    p = p * 2;     /* 2 の累乗 */
    printf("  2^%d = %ld\n", j, p);
    j++;
  }
  return 0;
}
