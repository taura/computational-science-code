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

   重み・画像は2次元配列 Mat (A(i,j) でアクセス)。ただし配列 reduction の対象 (勾配)
   は OpenMP の配列セクション構文に合わせて生のポインタ (gW1[...]) で扱う。
   入出力はNumPy標準の .npy 形式:
   - 読み: data/x_train.npy (uint8 [N,784]), data/y_train.npy (int32 [N])
   - 書き: data/W1.npy, b1.npy, W2.npy, b2.npy (float64) -> 00_mnist_infer が読む
   read_npy / write_npy は下に用意してある (I/O は本問題の主眼ではない)。 */

/* ---- 連続メモリの2次元配列 (row-major: A(i,j) = a[i*cols + j]) ---- */
struct Mat {
  long rows, cols;
  double * a;
  Mat(long r, long c)             : rows(r), cols(c), a(new double[r * c]) {}
  Mat(long r, long c, double * p) : rows(r), cols(c), a(p) {}   /* 既存バッファを引き取る */
  ~Mat() { delete[] a; }
  Mat(const Mat &) = delete;                  /* コピー禁止 (二重 delete を防ぐ) */
  Mat & operator=(const Mat &) = delete;
  double & operator()(long i, long j)       { return a[i * cols + j]; }
  double   operator()(long i, long j) const { return a[i * cols + j]; }
  double * data() { return a; }
};

/* ---- .npy 読み込み: 任意の数値型を double 配列に読み込む (C順, 1〜2次元) ---- */
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

/* ---- .npy 書き出し: double 配列を float64 の .npy として書く (C順, 1〜2次元) ---- */
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

/* 状態を持たない乱数 (初期値生成用): (seed,k) から [0,1)。 */
static inline double draw_rand01(long long seed, long long k) {
  const long long M = 2147483647LL;
  long long x = ((seed % M) * 2654435761LL + (k % M) + 1) % M;
  x = ((x ^ (x >> 16)) * 1812433253LL) % M;
  x = ((x ^ (x >> 13)) * 1664525LL)    % M;
  x =  (x ^ (x >> 16)) % M;
  return (double)x / (double)M;
}

int main(int argc, char ** argv) {
  int    E  = (argc > 1 ? atoi(argv[1]) : 20);    /* エポック数 */
  double lr = (argc > 2 ? atof(argv[2]) : 0.1);   /* 学習率 */
  int    BS = (argc > 3 ? atoi(argv[3]) : 100);   /* ミニバッチサイズ */
  const int IN = 784, HID = 128, OUT = 10;

  /* --- 訓練データの読み込み (画素 0..255 -> 0..1 に正規化) --- */
  long sx[2], sy[2]; int nd;
  double * xr = read_npy("data/x_train.npy", sx, &nd);   /* [N,784] */
  long N = sx[0];
  for (long i = 0; i < N * IN; i++) xr[i] /= 255.0;
  Mat X(N, IN, xr);
  double * yd = read_npy("data/y_train.npy", sy, &nd);   /* [N] */
  int * y = new int[N];
  for (long i = 0; i < N; i++) y[i] = (int)yd[i];
  delete[] yd;

  /* --- パラメータ初期化 (He 初期化, バイアスは 0) --- */
  Mat W1(HID, IN), W2(OUT, HID);
  double * b1 = new double[HID];
  double * b2 = new double[OUT];
  double s1 = sqrt(2.0 / IN), s2 = sqrt(2.0 / HID);
  for (long k = 0; k < (long)HID * IN; k++)  W1.data()[k] = (draw_rand01(k, 1) - 0.5) * 2.0 * s1;
  for (int k = 0; k < HID; k++)              b1[k] = 0.0;
  for (long k = 0; k < (long)OUT * HID; k++) W2.data()[k] = (draw_rand01(k, 2) - 0.5) * 2.0 * s2;
  for (int k = 0; k < OUT; k++)              b2[k] = 0.0;

  /* 勾配の総和を入れる配列。配列 reduction の対象なので生のポインタで扱う。 */
  double * gW1 = new double[HID * IN];
  double * gb1 = new double[HID];
  double * gW2 = new double[OUT * HID];
  double * gb2 = new double[OUT];

  double loss = 0.0; long correct = 0;
  double t0 = omp_get_wtime();
  for (int ep = 0; ep < E; ep++) {
    loss = 0.0; correct = 0;
    for (long b0 = 0; b0 < N; b0 += BS) {
      long b1n = (b0 + BS < N) ? b0 + BS : N;        /* バッチ [b0, b1n) */
      long m = b1n - b0;
      memset(gW1, 0, sizeof(double) * HID * IN);
      memset(gb1, 0, sizeof(double) * HID);
      memset(gW2, 0, sizeof(double) * OUT * HID);
      memset(gb2, 0, sizeof(double) * OUT);

      /* バッチ内の全サンプルにわたる forward + backprop。各サンプルの勾配寄与を総和。
         損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。 */
      // BEGIN ANSWER: バッチのループを配列 reduction で並列化せよ: #pragma omp parallel for reduction(+:loss,correct,gb2[:OUT],gW2[:OUT*HID],gb1[:HID],gW1[:HID*IN]).
#pragma omp parallel for reduction(+:loss,correct,gb2[:OUT],gW2[:OUT*HID],gb1[:HID],gW1[:HID*IN])
      // END ANSWER
      for (long i = b0; i < b1n; i++) {
        double h[128];                          /* HID=128 */
        for (int k = 0; k < HID; k++) {         /* h = ReLU(W1 x + b1) */
          double z = b1[k];
          for (int j = 0; j < IN; j++) z += W1(k, j) * X(i, j);
          h[k] = (z > 0.0) ? z : 0.0;
        }
        double o[10], omax = -1e300;            /* o = W2 h + b2 */
        for (int c = 0; c < OUT; c++) {
          double z = b2[c];
          for (int k = 0; k < HID; k++) z += W2(c, k) * h[k];
          o[c] = z; if (z > omax) omax = z;
        }
        double sum = 0.0;                        /* p = softmax(o) */
        for (int c = 0; c < OUT; c++) { o[c] = exp(o[c] - omax); sum += o[c]; }
        int best = 0; double bestv = -1.0;
        for (int c = 0; c < OUT; c++) {
          o[c] /= sum;
          if (o[c] > bestv) { bestv = o[c]; best = c; }
        }
        loss -= log(o[y[i]] + 1e-12);
        if (best == y[i]) correct++;
        /* backprop: do = p - onehot(y) */
        double dout[10];
        for (int c = 0; c < OUT; c++) dout[c] = o[c] - (c == y[i] ? 1.0 : 0.0);
        for (int c = 0; c < OUT; c++) {
          gb2[c] += dout[c];
          for (int k = 0; k < HID; k++) gW2[c * HID + k] += dout[c] * h[k];
        }
        for (int k = 0; k < HID; k++) {          /* dh = (W2^T do)・[h>0] */
          if (h[k] <= 0.0) continue;
          double dh = 0.0;
          for (int c = 0; c < OUT; c++) dh += W2(c, k) * dout[c];
          gb1[k] += dh;
          for (int j = 0; j < IN; j++) gW1[k * IN + j] += dh * X(i, j);
        }
      }

      /* 更新 (バッチ内勾配を平均して降下) */
      double sc = lr / (double)m;
      for (int k = 0; k < HID; k++) {
        for (int j = 0; j < IN; j++) W1(k, j) -= sc * gW1[k * IN + j];
        b1[k] -= sc * gb1[k];
      }
      for (int c = 0; c < OUT; c++) {
        for (int k = 0; k < HID; k++) W2(c, k) -= sc * gW2[c * HID + k];
        b2[c] -= sc * gb2[c];
      }
    }
    loss /= (double)N;
    if (ep % 5 == 0 || ep == E - 1)
      printf("epoch %4d: loss=%.4f, train acc=%.2f%%\n", ep, loss, 100.0 * correct / N);
  }
  double elapsed = omp_get_wtime() - t0;

  printf("最終: N=%ld, HID=%d, epochs=%d, loss=%.4f, train acc=%.2f%%\n",
         N, HID, E, loss, 100.0 * correct / N);
  printf("elapsed = %.3f sec\n", elapsed);

  /* --- 学習済みの重みを .npy で書き出す (00_mnist_infer が読む) --- */
  write_npy("data/W1.npy", W1.data(), HID, IN);
  write_npy("data/b1.npy", b1, HID, 0);
  write_npy("data/W2.npy", W2.data(), OUT, HID);
  write_npy("data/b2.npy", b2, OUT, 0);
  printf("重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました\n");

  delete[] b1; delete[] b2; delete[] y;
  delete[] gW1; delete[] gb1; delete[] gW2; delete[] gb2;
  return 0;
}
