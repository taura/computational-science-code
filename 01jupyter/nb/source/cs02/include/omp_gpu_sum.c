#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <omp.h>

// a function that calculates the sum of all elements of v
double sum(double * v, long m) {
  double s = 0.0;
/*** if VER >= 2 */
#pragma omp target teams distribute parallel for reduction(+:s) map(to: v, v.a[0:v.n]) map(tofrom: s)
/*** endif */
  for (long i = 0; i < m; i++) {
    s += v[i];
  }
  return s;
}

int main(int argc, char ** argv) {
  long m = (argc > 1 ? atof(argv[1]) : 1000000);
  double * v = (double *)calloc(sizeof(double), m);
  // init array (on CPU)
  for (long i = 0; i < m; i++) {
    v[i] = 1.0;
  }
  long t0 = omp_get_wtime();
  // get sum of the array (you make it happen on GPU)
  double s = sum(v, m);
  long t1 = omp_get_wtime();
  printf("pid = %d, answer = %f, took %ld ns\n",
         getpid(), s, t1 - t0);
  return 0;
}
