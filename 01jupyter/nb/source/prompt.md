static void forward(Net & net, int m) {
  dense_relu   (net.W1, net.X, net.b1, net.H, m);   /* H = ReLU(W1 X + b1) */
  dense_softmax(net.W2, net.H, net.b2, net.P, m);   /* P = softmax(W2 H + b2) */
}

dense_relu の中で行列積演算が発生するのがキモいんですが。
行列積（＋バイアス）は単独の関数にしてくださいよ。


for (int i = 0; i < m; i++) {
    matvec(net.W1, net.X[i], net.b1, net.H[i]); relu_inplace(net.H[i], HID);
    matvec(net.W2, net.H[i], net.b2, net.P[i]); softmax_inplace(net.P[i], OUT);
    loss -= log(net.P[i][net.y[i]] + 1e-12);
    if (argmax(net.P[i], OUT) == net.y[i]) correct++;
  }

は外側のループ自体をなくして、 

matmul(net.W1, net.X, net.b1, net.H);

とかやればいいのでは? そうやって各ステップがバッチ中の全部のサンプルをまとめて処理するようにして下さい。

backward もそうできるんじゃないの?

infer の方の

  for (int i = 0; i < NT; i++)
    if (predict(net, i) == (int)net.y[i]) correct++;

も同様。 predict 自体が全入力をまとめて処理するようにできるはず。


[1] infer の方

static int predict(const Net & net, int i) {
  double h[HID], o[OUT];
  matvec(net.W1, net.x[i], net.b1, h); relu_inplace(h, HID);
  matvec(net.W2, h, net.b2, o);
  return argmax(o, OUT);
}

あいからわず h, o を stack に割り当てているが、 net の中にそれらの行列がすでに割り当てられているという状態にして下さい。

[2] 

  int NT = (int)load_npy("data/x_test.npy", &net.x[0][0], -1, IN);   /* 枚数は不問 */
  load_npy("data/y_test.npy", net.y, NT, 0);
  x と y の非対称性が気になる。それらは MAXN でいいのでは? そのうえで実際のデータが MAXN より大きければ MAXN 個だけ読み込む。返り値は読み込んだデータの数、はOK。

[3] mlp_train の方

for (int k = 0; k < HID; k++) net.b1[k] = 0.0;
for (int c = 0; c < OUT; c++) net.b2[c] = 0.0;
このへんは、zero(net.b1 とかしたい。

[4] 
/* 今のバッチを net.X, net.y にコピー */
for (int i = 0; i < m; i++) {
  for (int j = 0; j < IN; j++) net.X[i][j] = Xall[(b0 + i) * IN + j];
  net.y[i] = yall[b0 + i];
}

この辺も関数化してほしい

[5] 

  long sh[2];
  read_npy("data/x_train.npy", nullptr, sh, 2);   /* dst=nullptr で形だけ取得 */
  long N = sh[0];
  double * Xall = new double[N * IN];
  read_npy("data/x_train.npy", Xall, sh, 2);
  for (long i = 0; i < N * IN; i++) Xall[i] /= 255.0;  /* 0..255 -> 0..1 */
  double * yd = new double[N];
  read_npy("data/y_train.npy", yd, sh, 1);
  int * yall = new int[N];
  for (long i = 0; i < N; i++) yall[i] = (int)yd[i];
  delete[] yd;

この辺全体が長ったらしくて嫌。適切に関数化。



mnist_infer.cpp

read_npy と load_npy の関係がややawkward
load_npy で double * dest を渡しているのにわざわざ raed_npy の中で new して copy して delete している。
最初から read_npy に dst を渡して read_npy がそこに書くようにすればよい。
限りなく、 load_npy = read_npy + 配列の大きさが予期していた通りかのチェックだけという風にしたい。

Net の中に x, y も入れてしまえばよい。
大きさは、 x[MAX_BATCH_SIZE][INPUT_DIMENSION] となることだろう（変数名はそちらで考えて）。
W, b と同じやり方で読み込めば良い。 load_npy("data/x_test.npy", net.x, -1, IN) みたいな感じ?
Netの中に x, y を入れれば predict に別途 x を入れる必要もない。

mlp_train.cpp 

Net, Grad, x 全て同じ Net に詰め込んでしまえば良い。
forward_backward が相変わらずたくさんの引数を受け取っているが、 Net だけになるはず（今でも Net, Grad, x, y だけにはできるはずだがそうではなくｔ Net だけにする）。


はいそうしてください。まずは mlp の 2件をそれで書いてみて下さい。

MLだけ違うというのはやや気になるところではありますがやっぱりそれでも魅力的です。
テンプレートは、 template<int R, int C> void matvec(double W[R][C]) ... と書けるはずじゃないかと思いますがどうでしょう? この程度のtemplateであれば、Matクラスでラップするよりむしろシンプルという気もしてきた。なぜフラット添字に逆戻りするのかが理解できてないです。
reduction できない問題は、 double * gW1 = g.gW1; みたいにして gw1 を渡すというのはできないの?
X だけは外になる問題ですが、max batch size 分だけを固定で確保するという方法があります。
サイズが固定になること自体はいかんともしがたいですね。
もう一つこのやり方がいい理由は GPU との相性がいいのではと? モデルをまるごとGPUにコピーするのが楽にできませんか?
確かに、折衷案もありえます。GPUとの相性はあまり良くないけど。
気になっているのは Fortran ではどうするのかという点。構造体に配列を詰め込むというのはどう書く?

案1 はまあいいと思うのだが、別の方法としては、中間配列を含めて全てを一つの構造体に入れて、predictもforward_backwardもその構造体一つを受け取るだけにする。大胆な話としては配列の大きさを定数にしてしまって、その構造体中に配列の本体も持たせてしまう。
異なるサイズの行列に対する行列積や行列ベクトル積を共通化するところでひと工夫が必要になるが(C++ だとテンプレートが使えるといえば使えるが、ちょっと初学者に対してヘビーな書き方が増えすぎるという嫌いもある)。

案1 と この案を比較検討してもらえるとありがたい。

まだところどころ、Vec と double * がまざっています。
例えば mlp_train.cpp で
static void matvec(const Mat & W, const double * b, const double * x, double * y);
となっていますが、一貫性という意味では b, x, y も Vec であってほしい。
ただ、Vec, Mat は, 値の配列を heap に割り当てる前提なので predict や forward_backward
などで中間行列をどこに割り当てるかという問題がでてくると思います。
毎回heapに割り当て、解放を繰り返したくないという問題もあります。

static int predict(const Mat & W1, const Vec & b1,
                   const Mat & W2, const Vec & b2, const double * x);
static LossCorrect forward_backward(const Mat & W1, const Vec & b1,
                                    const Mat & W2, const Vec & b2,
                                    const double * x, int label,
                                    double * gW1, double * gb1,
                                    double * gW2, double * gb2);
				   
どういう解決策があるか提案して。



static double * read_npy(const char * path, long shape[2], int * ndim);

ですが、ndim はむしろこちらで決めてわたす。 (int * -> int)
ファイルがそれにあっていなければエラーとする。
shape は long * shape として、ndim 個の要素を格納できるものとする。

というふうにしましょう。
それに合わせて、
static Mat read_npy_vec(const char * path);
static Mat read_npy_mat(const char * path);
も書き換える。

まだところどころ、Vec と double * がまざっています。
例えば mlp_train.cpp で
static void matvec(const Mat & W, const double * b, const double * x, double * y);
となっていますが、一貫性という意味では b, x, y も Vec であってほしい。
ただ、Vec, Mat は, 値の配列を heap に割り当てる前提なので



問題の背景に関してのお願いと質問

[1] 偏微分方程式を解いている問題では元の偏微分方程式と各記号の意味などを problem.md に書く
[2] 長くなりすぎない範囲で、そこから計算法を導く過程（例えば差分法であれば偏微分方程式を離散化するところ）も書く
[3] CG法も、どういう物理の問題を解いているのかがわかりにくいので説明を加える

[4] 群れの創発問題では、個々の鳥がどういうルールに従って動いているのでしょうか? これは説明に加える前に私に教えて
[5] 

vibration (固有振動モードを求める) の問題についてもう少し詳しく私向けに解説して下さい。なぜ
「四辺を固定した膜が自由に振動するとき, その**定在波 (固有振動モード)** `u(x,y)` は固有値問題

```
−∇²u = λ u,    境界で u = 0
```

を満たす。」のでしょうか? 

いきなり「`A` は **B1 (2D熱伝導) や G1 (CG) と全く同じ 2次元ラプラシアン行列**」というのが天下り的に感じる。
まず何をシミュレートしているのかの説明。 (正方形の膜？枠が固定？)
それと2次元ラプラシアン行列の関係は?
など。まず私に説明して。




CLAUDE.md の指示は理解しましたか?

C++/Fortran 入門は後回しにしてまず、それ以外のはじめのトピックである parallel, `omp_num_threads()`, `omp_thread_num()` についてトップレベルの部分だけ作ってみて（練習問題は後回し）。


いい感じです。次の for/collapse のトピックも作って。
あと以降、元のファイルで #PJM -L rscgrp=lecture7-a となっているところは、 #PJM -L rscgrp=lecture-a として下さい。（今回のはこちらで直しておきました）。

そうしよう。作る順番なんだけど、まず各トピックの一番易しい問題（プラグマやコメントを挿入すれば終り、みたいな問題）を一問ずつ作ろう。

その前にフォルダ名とファイルの配置について再考。
output の下ではなくこのディレクトリ直下としよう。
そして、トピックの順番にしたがってフォルダに名前をつける。
C++ Fortran 入門が 01_intro, parallel が 02_parallel という具合。
その中の .py のファイル名は変えなくて良い。
include のパス名も変えないといけない。m(_ _)m

ごめん、練習問題のパス名も変えましょう。今、  nn_トピック名/ex_問題名 になってますが、

nn_トピック名/problems/nn_問題名 みたいにしましょう。

たとえば

    02_parallel/ex_hello_threads
-> 02_parallel/problems/00_hello_threads

(問題番号は00から開始)





