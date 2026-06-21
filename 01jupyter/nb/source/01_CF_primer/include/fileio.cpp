#include <cstdio>

int main() {
  /* 書き込み: "w" で開く */
  FILE * wp = fopen("data.txt", "w");
  if (wp == NULL) { printf("cannot open for write\n"); return 1; }
  for (int i = 0; i < 5; i++) {
    fprintf(wp, "%d %f\n", i, i * 0.5);   /* 1行に2つの数を書く */
  }
  fclose(wp);

  /* 読み込み: "r" で開く */
  FILE * rp = fopen("data.txt", "r");
  if (rp == NULL) { printf("cannot open for read\n"); return 1; }
  int i;
  double x;
  /* fscanf が 2 個読めた間繰り返す */
  while (fscanf(rp, "%d %lf", &i, &x) == 2) {
    printf("read: i = %d, x = %f\n", i, x);
  }
  fclose(rp);
  return 0;
}
