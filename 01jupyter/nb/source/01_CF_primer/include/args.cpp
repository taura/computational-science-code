#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  /* argc は引数の個数 (プログラム名自身を含む)
     argv[0] はプログラム名, argv[1], argv[2], ... が実際の引数(文字列) */
  printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  /* 文字列を数値に変換する: atoi (整数), atof (浮動小数点) */
  int    n = (argc > 1 ? atoi(argv[1]) : 10);
  double x = (argc > 2 ? atof(argv[2]) : 1.0);
  printf("n = %d, x = %f\n", n, x);
  return 0;
}
