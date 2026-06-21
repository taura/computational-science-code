#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv) {
  long N = (argc > 1 ? atol(argv[1]) : 1000);
  /* 2 を何回かけたら N を超えるか (2^k > N となる最小の k) を求める */
  long p = 1;   /* p = 2^k */
  int k = 0;
  // TODO: p が N を超えるまで「p を 2 倍し k を 1 増やす」を繰り返す while ループを書け.
  // BEGIN ANSWER
  while (p <= N) {
    p = p * 2;
    k++;
  }
  // END ANSWER
  printf("2^%d = %ld is the first power of 2 greater than %ld\n", k, p, N);
  return 0;
}
