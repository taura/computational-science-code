/*** if VER == "add" */
void add(double * restrict x, double * restrict y, double * restrict z) {
#pragma omp simd
  for (int i = 0; i < 8; i++) {
    z[i] = x[i] + y[i];
  }
}
/*** endif */
/*** if VER in [ "doublev" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
/*** endif */
/*** if VER in [ "doublev_fma" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl), // double x 8
                                      aligned(sizeof(double))));
doublev doublev_fma(doublev a, doublev b, doublev c) {
  return a * b + c;
}
/*** endif */
/*** if VER in [ "doublev_fma_mixed" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev doublev_fma_mixed(double a, doublev b, doublev c) {
  return a * b + c;
}
/*** endif */
/*** if VER in [ "doublev_fma16" ] */
enum { nl = 16 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev doublev_fma16(doublev a, doublev b, doublev c) {
  return a * b + c;
}
/*** endif */
/*** if VER in [ "uniform" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev uniform(double u) {
  doublev v = { u,u,u,u,u,u,u,u };
  return v;
}
/*** endif */
/*** if VER in [ "uniform2" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev uniform2(double u) {
  doublev v;
  for (int i = 0; i < nl; i++) {
    v[i] = u;
  }
  return v;
}
/*** endif */
/*** if VER in [ "range" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev range(double u) {
  doublev v;
  for (int i = 0; i < nl; i++) {
    v[i] = u + i;
  }
  return v;
}
/*** endif */
/*** if VER in [ "loadv" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
doublev loadv(double * a) {
  doublev v;
  for (int i = 0; i < nl; i++) {
    v[i] = a[i];
  }
  return v;
}
/*** endif */
/*** if VER in [ "storev" ] */
enum { nl = 8 };
typedef double doublev __attribute__((vector_size(sizeof(double) * nl),
                                      aligned(sizeof(double))));
void storev(double * a, doublev v) {
  for (int i = 0; i < nl; i++) {
    a[i] = v[i];
  }
}
/*** endif */
