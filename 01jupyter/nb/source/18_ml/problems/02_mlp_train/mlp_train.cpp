#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <omp.h>

/* 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
   ネットワーク: 入力 784 (28x28画像) -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
   AI の「学習」の中身が行列積の繰り返しであることを体験する。

   forward:  h = ReLU(W1 x + b1),  o = W2 h + b2,  p = softmax(o)
   損失:     多クラスのクロスエントロピー
   backprop: do = p - onehot(y),  gW2 += do h^T, gb2 += do,
             dh = (W2^T do)・[h>0],  gW1 += dh x^T,  gb1 += dh
   更新:     ミニバッチ内の勾配を総和し, W -= lr * grad/batch。
   並列化対象は「ミニバッチ内の全サンプルにわたる勾配の和」(配列 reduction)。

   1サンプルの forward+backprop は forward_backward() に, 重み更新は sgd_update() に
   まとめてある。行列は Mat, ベクトルは Vec (中身は連続メモリで .a が生バッファ)。
   ただし配列 reduction の対象 (勾配) は, OpenMP の配列セクションが「素のポインタ」を
   base に要求するため, gW1.a 等を指すポインタを reduction の箇所でだけ使う。
   入出力はNumPy標準の .npy 形式 (read_npy / write_npy を用意)。 */

/* ====================== 行列 (2次元) と ベクトル (1次元) ====================== */
/* 連続メモリの row-major 行列: A(i,j) = a[i*cols + j] */
struct Mat {
  long rows, cols;
  double * a;
  Mat(long r, long c)             : rows(r), cols(c), a(new double[r * c]) {}
  Mat(long r, long c, double * p) : rows(r), cols(c), a(p) {}   /* 既存バッファを引き取る */
  Mat(Mat && o) noexcept : rows(o.rows), cols(o.cols), a(o.a) { o.a = nullptr; }
  ~Mat() { delete[] a; }
  Mat(const Mat &) = delete;                  /* コピー禁止 (二重 delete を防ぐ) */
  Mat & operator=(const Mat &) = delete;
  double & operator()(long i, long j)       { return a[i * cols + j]; }
  double   operator()(long i, long j) const { return a[i * cols + j]; }
  void zero() { for (long i = 0; i < rows * cols; i++) a[i] = 0.0; }
};

struct Vec {
  long n;
  double * a;
  Vec(long n_)             : n(n_), a(new double[n_]) {}
  Vec(long n_, double * p) : n(n_), a(p) {}                     /* 既存バッファを引き取る */
  Vec(Vec && o) noexcept : n(o.n), a(o.a) { o.a = nullptr; }
  ~Vec() { delete[] a; }
  Vec(const Vec &) = delete;
  Vec & operator=(const Vec &) = delete;
  double & operator()(long i)       { return a[i]; }
  double   operator()(long i) const { return a[i]; }
  void zero() { for (long i = 0; i < n; i++) a[i] = 0.0; }
};

/* ====================== .npy 入出力 ====================== */
/* .npy を読み, 任意の数値型を double 配列に読み込む (C順, 1〜2次元) */
static double * read_npy(const char * path, long shape[2], int * ndim) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);           /* ヘッダ(辞書文字列)の長さ */
  char * hdr = new char[hlen + 1];
  fread(hdr, 1, hlen, f); hdr[hlen] = '\0';
  /* dtype (例: '<f8','|u1','<i4','<i8') と shape をヘッダ文字列から取り出す */
  char descr[8] = {0};
  { char * q = strstr(hdr, "descr"); q = strchr(q, ':'); q = strchr(q, '\'') + 1;
    int i = 0; while (*q != '\'' && i < 7) descr[i++] = *q++; descr[i] = '\0'; }
  long s0 = 1, s1 = 1; *ndim = 1;
  char * sp = strstr(hdr, "shape"); sp = strchr(sp, '(') + 1;
  s0 = atol(sp);
  char * comma = strchr(sp, ',');
  char * after = comma + 1; while (*after == ' ') after++;
  if (*after != ')') { s1 = atol(after); *ndim = 2; } else { s1 = 1; *ndim = 1; }
  shape[0] = s0; shape[1] = s1;
  long n = s0 * (*ndim == 2 ? s1 : 1);
  delete[] hdr;

  double * out = new double[n];
  if (!strcmp(descr, "<f8")) {
    fread(out, sizeof(double), n, f);
  } else if (!strcmp(descr, "<f4")) {
    float * t = new float[n]; fread(t, sizeof(float), n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "|u1")) {
    unsigned char * t = new unsigned char[n]; fread(t, 1, n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i4")) {
    int * t = new int[n]; fread(t, sizeof(int), n, f);
    for (long i = 0; i < n; i++) { out[i] = t[i]; } delete[] t;
  } else if (!strcmp(descr, "<i8")) {
    long long * t = new long long[n]; fread(t, sizeof(long long), n, f);
    for (long i = 0; i < n; i++) { out[i] = (double)t[i]; } delete[] t;
  } else {
    printf("%s: 未対応の dtype %s\n", path, descr); exit(1);
  }
  fclose(f);
  return out;
}

/* .npy を行列として読む (1次元なら cols=1) */
static Mat read_npy_mat(const char * path) {
  long sh[2]; int nd;
  double * a = read_npy(path, sh, &nd);
  return Mat(sh[0], (nd == 2 ? sh[1] : 1), a);
}

/* .npy をベクトルとして読む */
static Vec read_npy_vec(const char * path) {
  long sh[2]; int nd;
  double * a = read_npy(path, sh, &nd);
  return Vec(sh[0] * (nd == 2 ? sh[1] : 1), a);
}

/* 画像 .npy (uint8 0..255) を読み, 0..1 に正規化した行列 [N,784] を返す */
static Mat load_images(const char * path) {
  Mat X = read_npy_mat(path);
  for (long i = 0; i < X.rows * X.cols; i++) X.a[i] /= 255.0;
  return X;
}

/* double 配列を float64 の .npy として書き出す (C順, s1=0 なら1次元) */
static void write_npy(const char * path, const double * data, long s0, long s1) {
  FILE * f = fopen(path, "wb");
  if (!f) { printf("%s が書けません\n", path); exit(1); }
  char shape[64];
  if (s1 > 0) snprintf(shape, sizeof(shape), "(%ld, %ld)", s0, s1);
  else        snprintf(shape, sizeof(shape), "(%ld,)", s0);
  char dict[128];
  int dn = snprintf(dict, sizeof(dict),
                    "{'descr': '<f8', 'fortran_order': False, 'shape': %s, }", shape);
  int total = 10 + dn + 1;                     /* +1 は末尾の改行 */
  int pad = (64 - (total % 64)) % 64;          /* 全体を64の倍数に揃える */
  int hlen = dn + 1 + pad;
  unsigned char head[10] = {0x93,'N','U','M','P','Y',1,0,
                            (unsigned char)(hlen & 0xff),(unsigned char)(hlen >> 8)};
  fwrite(head, 1, 10, f);
  fwrite(dict, 1, dn, f);
  for (int i = 0; i < pad; i++) fputc(' ', f);
  fputc('\n', f);
  long n = s0 * (s1 > 0 ? s1 : 1);
  fwrite(data, sizeof(double), n, f);
  fclose(f);
}

/* ====================== 行列演算・活性化 ====================== */
/* 行列ベクトル積 + バイアス: y = W x + b  (W: rows×cols, x: cols, y: rows) */
static void matvec(const Mat & W, const double * b, const double * x, double * y) {
  for (long i = 0; i < W.rows; i++) {
    double s = b[i];
    for (long j = 0; j < W.cols; j++) s += W(i, j) * x[j];
    y[i] = s;
  }
}

/* 転置行列ベクトル積: y = W^T d  (W: rows×cols, d: rows, y: cols) */
static void matTvec(const Mat & W, const double * d, double * y) {
  for (long j = 0; j < W.cols; j++) y[j] = 0.0;
  for (long i = 0; i < W.rows; i++)
    for (long j = 0; j < W.cols; j++) y[j] += W(i, j) * d[i];
}

/* 外積の加算 (rank-1 更新): G += a b^T  (G: m×n の連続配列, a: m, b: n) */
static void add_outer(double * G, long m, long n, const double * a, const double * b) {
  for (long i = 0; i < m; i++)
    for (long j = 0; j < n; j++) G[i * n + j] += a[i] * b[j];
}

/* ベクトルの加算: acc += v */
static void vec_add(double * acc, const double * v, long n) {
  for (long i = 0; i < n; i++) acc[i] += v[i];
}

/* y += s x  (axpy) */
static void axpy(double * y, double s, const double * x, long n) {
  for (long i = 0; i < n; i++) y[i] += s * x[i];
}

static void relu_inplace(double * v, long n) {
  for (long i = 0; i < n; i++) if (v[i] < 0.0) v[i] = 0.0;
}

static void softmax_inplace(double * v, long n) {
  double mx = v[0];
  for (long i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
  double s = 0.0;
  for (long i = 0; i < n; i++) { v[i] = exp(v[i] - mx); s += v[i]; }
  for (long i = 0; i < n; i++) v[i] /= s;
}

static int argmax(const double * v, long n) {
  int best = 0;
  for (long i = 1; i < n; i++) if (v[i] > v[best]) best = (int)i;
  return best;
}

/* ====================== 学習の心臓部 ====================== */
/* 状態を持たない乱数 (初期値生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

/* He 初期化 (バイアスは別途 0 にする) */
static void init_he(Mat & W, long long salt) {
  double scale = sqrt(2.0 / W.cols) * 2.0;
  for (long k = 0; k < W.rows * W.cols; k++)
    W.a[k] = (draw_rand01(k, salt) - 0.5) * scale;
}

/* 1サンプルの forward + backprop。勾配を g... に加算し, 損失と正否を返す。
   勾配は配列 reduction の対象なので生ポインタで受け取り gW1[...] で加算する。 */
struct LossCorrect { double loss; int correct; };
static LossCorrect forward_backward(const Mat & W1, const Vec & b1,
                                    const Mat & W2, const Vec & b2,
                                    const double * x, int label,
                                    double * gW1, double * gb1,
                                    double * gW2, double * gb2) {
  const int HID = W1.rows, IN = W1.cols, OUT = W2.rows;
  double h[1024], o[64], dout[64], dh[1024];       /* HID<=1024, OUT<=64 を仮定 */
  matvec(W1, b1.a, x, h); relu_inplace(h, HID);    /* h = ReLU(W1 x + b1) */
  matvec(W2, b2.a, h, o); softmax_inplace(o, OUT); /* p = softmax(W2 h + b2) */

  LossCorrect r;
  r.loss = -log(o[label] + 1e-12);
  r.correct = (argmax(o, OUT) == label) ? 1 : 0;

  for (int c = 0; c < OUT; c++) dout[c] = o[c] - (c == label ? 1.0 : 0.0); /* do = p - onehot */
  vec_add(gb2, dout, OUT);                         /* gb2 += do            */
  add_outer(gW2, OUT, HID, dout, h);               /* gW2 += do h^T        */
  matTvec(W2, dout, dh);                           /* dh = W2^T do         */
  for (int k = 0; k < HID; k++) if (h[k] <= 0.0) dh[k] = 0.0;  /* ReLU の微分 (・[h>0]) */
  vec_add(gb1, dh, HID);                           /* gb1 += dh            */
  add_outer(gW1, HID, IN, dh, x);                  /* gW1 += dh x^T        */
  return r;
}

/* 勾配降下の1ステップ: W -= sc * gW,  b -= sc * gb */
static void sgd_update(Mat & W, Vec & b, const Mat & gW, const Vec & gb, double sc) {
  axpy(W.a, -sc, gW.a, W.rows * W.cols);
  axpy(b.a, -sc, gb.a, b.n);
}

/* ====================== main ====================== */
int main(int argc, char ** argv) {
  int    E  = (argc > 1 ? atoi(argv[1]) : 20);    /* エポック数 */
  double lr = (argc > 2 ? atof(argv[2]) : 0.1);   /* 学習率 */
  int    BS = (argc > 3 ? atoi(argv[3]) : 100);   /* ミニバッチサイズ */

  /* 訓練データの読み込み */
  Mat X = load_images("data/x_train.npy");        /* [N, IN], 0..1 正規化済み */
  Vec y = read_npy_vec("data/y_train.npy");
  const int IN = X.cols, HID = 128, OUT = 10;
  long N = X.rows;

  /* パラメータ初期化 (He 初期化, バイアスは 0) */
  Mat W1(HID, IN), W2(OUT, HID);
  Vec b1(HID), b2(OUT);
  init_he(W1, 1); init_he(W2, 2);
  b1.zero(); b2.zero();

  /* 勾配の総和を入れる配列 (重みと同じ形なので gW は Mat, gb は Vec) */
  Mat gW1(HID, IN), gW2(OUT, HID);
  Vec gb1(HID), gb2(OUT);
  /* 配列 reduction の配列セクションは base が素のポインタ識別子である必要があるため,
     中身を指すポインタを reduction の箇所でだけ使う (長さには .n / .rows*.cols を使う)。 */
  double * gW1a = gW1.a, * gW2a = gW2.a, * gb1a = gb1.a, * gb2a = gb2.a;

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0; correct = 0;
    for (long b0 = 0; b0 < N; b0 += BS) {
      long b1n = (b0 + BS < N) ? b0 + BS : N;      /* バッチ [b0, b1n) */
      long m = b1n - b0;
      gW1.zero(); gW2.zero(); gb1.zero(); gb2.zero();

      /* バッチ内の各サンプルの勾配寄与を総和する。各サンプルは独立。
         損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。 */
      // BEGIN ANSWER: バッチのループを配列 reduction で並列化せよ: #pragma omp parallel for reduction(+:loss,correct,gb2a[:gb2.n],gW2a[:gW2.rows*gW2.cols],gb1a[:gb1.n],gW1a[:gW1.rows*gW1.cols]).
#pragma omp parallel for \
        reduction(+:loss,correct,gb2a[:gb2.n],gW2a[:gW2.rows*gW2.cols],gb1a[:gb1.n],gW1a[:gW1.rows*gW1.cols])
      // END ANSWER
      for (long i = b0; i < b1n; i++) {
        LossCorrect r = forward_backward(W1, b1, W2, b2, &X(i, 0), (int)y(i),
                                         gW1a, gb1a, gW2a, gb2a);
        loss += r.loss; correct += r.correct;
      }

      double sc = lr / (double)m;                  /* バッチ内勾配を平均して降下 */
      sgd_update(W1, b1, gW1, gb1, sc);
      sgd_update(W2, b2, gW2, gb2, sc);
    }
    loss /= (double)N;
    if (ep % 5 == 0 || ep == E - 1)
      printf("epoch %4d: loss=%.4f, train acc=%.2f%%\n", ep, loss, 100.0 * correct / N);
  }
  double elapsed = omp_get_wtime() - t0;

  printf("最終: N=%ld, HID=%d, epochs=%d, loss=%.4f, train acc=%.2f%%\n",
         N, HID, E, loss, 100.0 * correct / N);
  printf("elapsed = %.3f sec\n", elapsed);

  /* 学習済みの重みを .npy で書き出す (00_mnist_infer が読む) */
  write_npy("data/W1.npy", W1.a, HID, IN);
  write_npy("data/b1.npy", b1.a, HID, 0);
  write_npy("data/W2.npy", W2.a, OUT, HID);
  write_npy("data/b2.npy", b2.a, OUT, 0);
  printf("重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました\n");
  return 0;
}
