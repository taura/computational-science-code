#include <cstdio>

typedef double doublev __attribute__((vector_size(64)));

/* ベクトル型の値 v を double の配列 a の先頭 8 要素 a[0]..a[7] に書き込む */
void storev(double *a, doublev v) {
  *(doublev *)a = v;
}
