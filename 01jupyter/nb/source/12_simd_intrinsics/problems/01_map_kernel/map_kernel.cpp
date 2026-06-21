#include <cstdio>
#include <cstdlib>

enum { nl = 8 };   /* double 8つ = 512 bit のベクトル長 */
typedef double doublev __attribute__((vector_size(sizeof(double) * nl)));

int main(int argc, char ** argv) {
  /* n は簡単のため nl の倍数とする */
  long n = (argc > 1 ? atol(argv[1]) : 64);
  double * x = (double *)malloc(sizeof(double) * n);
  double * y = (double *)malloc(sizeof(double) * n);
  for (long i = 0; i < n; i++) x[i] = i;

  /* y[i] = 2*x[i] + 1 を, nl 個ずつまとめて (ベクトル型で) 計算する */
  for (long i = 0; i < n; i += nl) {
    doublev xv = *(doublev *)&x[i];   /* x[i..i+nl-1] を1つのベクトルとして読む */
    doublev yv;
    // TODO: xv を使い y = 2*x + 1 を「ベクトルのまま」計算して yv に求めよ.
    // BEGIN ANSWER
    yv = 2.0 * xv + 1.0;
    // END ANSWER
    *(doublev *)&y[i] = yv;           /* y[i..i+nl-1] に書き戻す */
  }

  long err = 0;
  for (long i = 0; i < n; i++) if (y[i] != 2 * x[i] + 1) err++;
  if (err == 0) printf("OK: y[0]=%.1f, y[%ld]=%.1f\n", y[0], n - 1, y[n - 1]);
  else          printf("NG: %ld 要素が不正\n", err);
  free(x); free(y);
  return 0;
}
