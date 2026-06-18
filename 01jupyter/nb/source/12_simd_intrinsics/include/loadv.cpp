#include <cstdio>

typedef double doublev __attribute__((vector_size(64)));

/* double の配列 a の先頭 8 要素 a[0]..a[7] をまとめて
   ひとつのベクトル型の値として取り出す */
doublev loadv(double *a) {
  return *(doublev *)a;
}
