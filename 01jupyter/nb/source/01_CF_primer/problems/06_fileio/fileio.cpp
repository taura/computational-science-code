#include <cstdio>

int main() {
  /* まず data.txt に 0..4 の i と i*0.5 を書き出す (この部分は完成済み) */
  FILE * wp = fopen("data.txt", "w");
  if (wp == NULL) { printf("cannot open for write\n"); return 1; }
  for (int i = 0; i < 5; i++) {
    fprintf(wp, "%d %f\n", i, i * 0.5);
  }
  fclose(wp);

  /* 次に data.txt を読み直し, 2列目 (x) の合計を求める */
  FILE * rp = fopen("data.txt", "r");
  if (rp == NULL) { printf("cannot open for read\n"); return 1; }
  int i;
  double x;
  double sum = 0.0;
  // TODO: fscanf で1行ずつ (i, x) を読み, x を sum に足し込むループを書け.
  // BEGIN ANSWER
  while (fscanf(rp, "%d %lf", &i, &x) == 2) {
    sum += x;
  }
  // END ANSWER
  fclose(rp);
  printf("sum of x = %f\n", sum);
  return 0;
}
