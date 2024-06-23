#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <unistd.h>

/*** if VER == "mp" */
enum { nl = 16 };

typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));

doublev uniform(double u) {
  doublev v;
  for (long i = 0; i < nl; i++) {
    v[i] = u;
  }
  return v;
}

doublev lin(double u) {
  doublev v;
  for (long i = 0; i < nl; i++) {
    v[i] = u + i;
  }
  return v;
}

long count_lt(doublev a, doublev b) {
  long s = 0;
  for (int i = 0; i < nl; i++) {
    s += (a[i] < b[i]);
  }
  return s;
}

double volume_of_ball(long n, int nteams, int nthreads) {
  double h = 1.0 / (double)n;
  long s = 0;
#pragma omp parallel for num_threads(nthreads) collapse(2) reduction(+:s)
  for (long i = 0; i < n; i++) {
    for (long j = 0; j < n; j++) {
      double x = (i + 0.5) * h;
      double y = (j + 0.5) * h;
      for (long k = 0; k < n; k += nl) {
        doublev z = lin(k + 0.5) * h;
        s += count_lt(x * x + y * y + z * z, uniform(1.0));
      }
    }
  }
  return s * h * h * h;
}
/*** else */
double volume_of_ball(long n, int nteams, int nthreads) {
  double h = 1.0 / (double)n;
  long s = 0;
/*** if VER == "gpu" */
#pragma omp target teams distribute num_teams(nteams) parallel for num_threads(nthreads) collapse(2) map(tofrom: s) reduction(+:s)
/*** endif */
  for (long i = 0; i < n; i++) {
    for (long j = 0; j < n; j++) {
      for (long k = 0; k < n; k++) {
        double x = (i + 0.5) * h;
        double y = (j + 0.5) * h;
        double z = (k + 0.5) * h;
        s += (x * x + y * y + z * z < 1.0);
      }
    }
  }
  return s * h * h * h;
}
/*** endif */

int main(int argc, char ** argv) {
  long n           = (1 < argc ? atol(argv[1]) : 100);
  char * nteams_   = getenv("OMP_NUM_TEAMS");
  int    nteams    = (nteams_   ? atoi(nteams_) : 1);
  char * nthreads_ = getenv("OMP_NUM_THREADS");
  int    nthreads  = (nthreads_ ? atoi(nthreads_) : 1);

  printf("n             : %ld\n", n);
  printf("nteams        : %d\n", nteams);
  printf("nthreads      : %d\n", nthreads);
  /* 計測開始 */
  double t0 = omp_get_wtime();
  /* 計算本体 */
  double v = volume_of_ball(n, nteams, nthreads);
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */
  double error = fabs(v - M_PI/6.0);
  if (error > 1.0e-2) {
    fprintf(stderr, "WARNING: error (%f) > 0.01\n", error);
    fprintf(stderr, "check your program\n");
  }
  printf("volume        : %.9f\n", v);
  printf("error         : %e\n", );
  printf("elapsed       : %7.3f\n", dt);
  printf("n^3 / nsec    : %7.3f\n",
         (double)n * (double)n * (double)n / dt * 1.0e-9);
  return 0;
}
