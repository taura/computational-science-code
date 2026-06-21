#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <omp.h>

/* 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
   ネットワーク: 入力 784 (28x28画像) -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。

   forward:  h = ReLU(W1 x + b1),  o = W2 h + b2,  p = softmax(o)
   損失:     多クラスのクロスエントロピー
   backprop: do = p - onehot(y),  gW2 += do h^T, gb2 += do,
             dh = (W2^T do)・[h>0],  gW1 += dh x^T,  gb1 += dh
   更新:     ミニバッチ内の勾配を総和し, W -= lr * grad/batch。
   並列化対象は「ミニバッチ内の全サンプルにわたる勾配の和」(配列 reduction)。

   ネットワークの大きさは定数 (IN/HID/OUT) なので, パラメータは固定サイズ配列を
   まとめた構造体 Net, 勾配は同じく Grad にまとめる (ヒープ確保なし, 本物の2次元配列)。
   Net は内部にポインタを持たないので GPU 発展で map(to: net) がそのまま使える。
   配列 reduction は OpenMP の配列セクションが素のポインタを要求するので, Grad の
   中身を指すポインタ (gW1 等) を reduction の箇所でだけ使う。 */

static const int IN = 784, HID = 128, OUT = 10;

struct Net  { double W1[HID][IN], b1[HID], W2[OUT][HID], b2[OUT]; };
struct Grad {
  double gW1[HID][IN], gb1[HID], gW2[OUT][HID], gb2[OUT];
  void zero() {                                  /* ループでゼロ初期化 */
    for (int k = 0; k < HID; k++) { gb1[k] = 0.0; for (int j = 0; j < IN; j++)  gW1[k][j] = 0.0; }
    for (int c = 0; c < OUT; c++) { gb2[c] = 0.0; for (int k = 0; k < HID; k++) gW2[c][k] = 0.0; }
  }
};

/* ====================== .npy 入出力 ====================== */
/* .npy を読み, 任意の数値型を double 配列に読み込む (C順)。次元数 ndim は呼び出し側が
   指定し, ファイルの次元がそれと違えばエラーにする。shape には ndim 個の要素を格納する。 */
static double * read_npy(const char * path, long * shape, int ndim) {
  FILE * f = fopen(path, "rb");
  if (!f) { printf("%s が開けません\n", path); exit(1); }
  unsigned char magic[10];
  if (fread(magic, 1, 10, f) != 10 || memcmp(magic, "\x93NUMPY", 6) != 0) {
    printf("%s は .npy ではありません\n", path); exit(1);
  }
  int hlen = magic[8] | (magic[9] << 8);           /* ヘッダ(辞書文字列)の長さ */
  char * hdr = new char[hlen + 1];
  fread(hdr, 1, hlen, f); hdr[hlen] = '\0';
  char descr[8] = {0};
  { char * q = strstr(hdr, "descr"); q = strchr(q, ':'); q = strchr(q, '\'') + 1;
    int i = 0; while (*q != '\'' && i < 7) descr[i++] = *q++; descr[i] = '\0'; }
  long s0 = 1, s1 = 1; int file_ndim;
  char * sp = strstr(hdr, "shape"); sp = strchr(sp, '(') + 1;
  s0 = atol(sp);
  char * comma = strchr(sp, ',');
  char * after = comma + 1; while (*after == ' ') after++;
  if (*after != ')') { s1 = atol(after); file_ndim = 2; } else { file_ndim = 1; }
  if (file_ndim != ndim) {
    printf("%s: %d 次元を期待しましたが %d 次元でした\n", path, ndim, file_ndim); exit(1);
  }
  shape[0] = s0;
  long n = s0;
  if (ndim == 2) { shape[1] = s1; n *= s1; }
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
/* 行列ベクトル積 + バイアス: y = W x + b  (W は R×C, サイズは型から自動で決まる) */
template <int R, int C>
static void matvec(const double (&W)[R][C], const double * x,
                   const double (&b)[R], double (&y)[R]) {
  for (int i = 0; i < R; i++) {
    double s = b[i];
    for (int j = 0; j < C; j++) s += W[i][j] * x[j];
    y[i] = s;
  }
}

/* 転置行列ベクトル積: y = W^T d  (W は R×C, d は R, y は C) */
template <int R, int C>
static void matTvec(const double (&W)[R][C], const double * d, double * y) {
  for (int j = 0; j < C; j++) y[j] = 0.0;
  for (int i = 0; i < R; i++)
    for (int j = 0; j < C; j++) y[j] += W[i][j] * d[i];
}

/* 外積の加算 (rank-1 更新): G += a b^T  (G は m×n の連続配列, a は m, b は n) */
static void add_outer(double * G, int m, int n, const double * a, const double * b) {
  for (int i = 0; i < m; i++)
    for (int j = 0; j < n; j++) G[i * n + j] += a[i] * b[j];
}

static void vec_add(double * acc, const double * v, int n) {   /* acc += v */
  for (int i = 0; i < n; i++) acc[i] += v[i];
}

static void axpy(double * y, double s, const double * x, long n) {   /* y += s x */
  for (long i = 0; i < n; i++) y[i] += s * x[i];
}

static void relu_inplace(double * v, int n) {
  for (int i = 0; i < n; i++) if (v[i] < 0.0) v[i] = 0.0;
}

static void softmax_inplace(double * v, int n) {
  double mx = v[0];
  for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
  double s = 0.0;
  for (int i = 0; i < n; i++) { v[i] = exp(v[i] - mx); s += v[i]; }
  for (int i = 0; i < n; i++) v[i] /= s;
}

static int argmax(const double * v, int n) {
  int best = 0;
  for (int i = 1; i < n; i++) if (v[i] > v[best]) best = i;
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

/* He 初期化 (重み行列 w を fan-in n_in に応じた乱数で埋める) */
static void init_he(double * w, long total, int n_in, long long salt) {
  double scale = sqrt(2.0 / n_in) * 2.0;
  for (long k = 0; k < total; k++) w[k] = (draw_rand01(k, salt) - 0.5) * scale;
}

/* 1サンプルの forward + backprop。勾配を g... に加算し, 損失と正否を返す。
   勾配は配列 reduction の対象なので素のポインタで受け取り gW1[...] で加算する。 */
struct LossCorrect { double loss; int correct; };
static LossCorrect forward_backward(const Net & net, const double * x, int label,
                                    double * gW1, double * gb1,
                                    double * gW2, double * gb2) {
  double h[HID], o[OUT], dout[OUT], dh[HID];
  matvec(net.W1, x, net.b1, h); relu_inplace(h, HID);     /* h = ReLU(W1 x + b1) */
  matvec(net.W2, h, net.b2, o); softmax_inplace(o, OUT);  /* p = softmax(W2 h + b2) */

  LossCorrect r;
  r.loss = -log(o[label] + 1e-12);
  r.correct = (argmax(o, OUT) == label) ? 1 : 0;

  for (int c = 0; c < OUT; c++) dout[c] = o[c] - (c == label ? 1.0 : 0.0); /* do = p - onehot */
  vec_add(gb2, dout, OUT);                         /* gb2 += do            */
  add_outer(gW2, OUT, HID, dout, h);               /* gW2 += do h^T        */
  matTvec(net.W2, dout, dh);                       /* dh = W2^T do         */
  for (int k = 0; k < HID; k++) if (h[k] <= 0.0) dh[k] = 0.0;  /* ReLU の微分 (・[h>0]) */
  vec_add(gb1, dh, HID);                           /* gb1 += dh            */
  add_outer(gW1, HID, IN, dh, x);                  /* gW1 += dh x^T        */
  return r;
}

/* 勾配降下の1ステップ: W -= sc * gW,  b -= sc * gb */
static void sgd_update(Net & net, const double * gW1, const double * gb1,
                       const double * gW2, const double * gb2, double sc) {
  axpy(&net.W1[0][0], -sc, gW1, (long)HID * IN);  axpy(net.b1, -sc, gb1, HID);
  axpy(&net.W2[0][0], -sc, gW2, (long)OUT * HID); axpy(net.b2, -sc, gb2, OUT);
}

/* ====================== main ====================== */
int main(int argc, char ** argv) {
  int    E  = (argc > 1 ? atoi(argv[1]) : 20);    /* エポック数 */
  double lr = (argc > 2 ? atof(argv[2]) : 0.1);   /* 学習率 */
  int    BS = (argc > 3 ? atoi(argv[3]) : 100);   /* ミニバッチサイズ */

  /* 訓練データ (枚数 N は可変なので動的配列) を読み, 画素を 0..1 に正規化 */
  long sh[2];
  double * X = read_npy("data/x_train.npy", sh, 2);   /* [N, IN] */
  long N = sh[0];
  for (long i = 0; i < N * IN; i++) X[i] /= 255.0;
  double * yd = read_npy("data/y_train.npy", sh, 1);
  int * y = new int[N];
  for (long i = 0; i < N; i++) y[i] = (int)yd[i];
  delete[] yd;

  /* パラメータ初期化 (He 初期化, バイアスは 0) */
  Net net;
  init_he(&net.W1[0][0], (long)HID * IN, IN, 1);
  init_he(&net.W2[0][0], (long)OUT * HID, HID, 2);
  for (int k = 0; k < HID; k++) net.b1[k] = 0.0;
  for (int c = 0; c < OUT; c++) net.b2[c] = 0.0;

  /* 勾配は Grad 構造体。配列 reduction の句では中身を指すポインタを使う。 */
  Grad g;
  double * gW1 = &g.gW1[0][0], * gb1 = g.gb1, * gW2 = &g.gW2[0][0], * gb2 = g.gb2;

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0; correct = 0;
    for (long b0 = 0; b0 < N; b0 += BS) {
      long b1n = (b0 + BS < N) ? b0 + BS : N;      /* バッチ [b0, b1n) */
      long m = b1n - b0;
      g.zero();

      /* バッチ内の各サンプルの勾配寄与を総和する。各サンプルは独立。
         損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。 */
      // TODO: このバッチ内のループを並列化する (各サンプルは独立)。
      // BEGIN ANSWER
#pragma omp parallel for \
        reduction(+:loss,correct,gb2[:OUT],gW2[:OUT*HID],gb1[:HID],gW1[:HID*IN])
      // END ANSWER
      for (long i = b0; i < b1n; i++) {
        LossCorrect r = forward_backward(net, &X[i * IN], y[i], gW1, gb1, gW2, gb2);
        loss += r.loss; correct += r.correct;
      }

      double sc = lr / (double)m;                  /* バッチ内勾配を平均して降下 */
      sgd_update(net, gW1, gb1, gW2, gb2, sc);
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
  write_npy("data/W1.npy", &net.W1[0][0], HID, IN);
  write_npy("data/b1.npy", net.b1, HID, 0);
  write_npy("data/W2.npy", &net.W2[0][0], OUT, HID);
  write_npy("data/b2.npy", net.b2, OUT, 0);
  printf("重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました\n");

  delete[] X; delete[] y;
  return 0;
}
