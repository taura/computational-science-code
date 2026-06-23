#include <cstdio>
#include <cstdlib>

int is_prime(long k) {
  if (k < 2) return 0;
  for (long d = 2; d * d <= k; d++) {
    if (k % d == 0) return 0;
  }
  return 1;
}

int main(int argc, char ** argv) {
  long N = (argc > 1 ? atol(argv[1]) : 8000000L);
  long count = 0;
  // TODO: 下のループを #pragma omp parallel for schedule(runtime) reduction(+:count) で並列化せよ.
  // BEGIN ANSWER
#pragma omp parallel for schedule(runtime) reduction(+:count)
  // END ANSWER
  for (long i = 2; i <= N; i++) {
    count += is_prime(i);
  }
  printf("number of primes <= %ld : %ld\n", N, count);
  return 0;
}
