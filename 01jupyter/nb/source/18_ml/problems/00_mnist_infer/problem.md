# 応用問題: MNIST 手書き数字の認識 (行列積 = AI推論)

## 目標

**ニューラルネットワークの推論 (forward) の正体は「行列積 + 活性化関数」** である。これまで何度も並列化してきた行列(ベクトル)積が, そのまま AI の推論計算になっていることを, **本物の MNIST 手書き数字**を認識して体感する。

題材は 2層 MLP:

- 入力 784次元 (28×28 画像を一列に並べたもの) -> 隠れ層 128 (ReLU) -> 出力 10クラス
- `h = ReLU(W1 x + b1)` (128次元), `o = W2 h + b2` (10次元), 予測クラス = `argmax(o)`

学習済みの重みとテスト画像 (いずれも NumPy 標準の **`.npy` 形式**: ヘッダ + 生バイナリ) は Wisteria の共有領域に置いてある。冒頭の「データと重みの準備」セルで張った symlink 経由で読む:

- `weights/W1.npy, b1.npy, W2.npy, b2.npy` : 学習済みの重み (float64)
- `data/x_test.npy` : MNIST テスト画像 (各 784 画素, uint8 で 0..255)
- `data/y_test.npy` : 正解ラベル (int32, 0..9)

重みは別問題 **MLP に MNIST を学習させる (18_ml/problems/02_mlp_train)** が学習して書き出したもの。テスト画像は世の中で配布されている MNIST (`mnist.npz`) から取り出したもの。`.npy` の読み込み関数 `read_npy` はソース内に用意済みなので, **入出力を書く必要はない** (並列化に集中せよ)。

## 課題

`mnist_infer.cpp` (または `mnist_infer.f90`) は, 重みと画像を読み込み, 各画像を forward して予測クラスを求め, 正解率を表示する。各画像の推論は互いに**独立**なので並列化できる。

`TODO` の箇所に **OpenMP の指示を追加**し, 画像のループを並列化せよ。

- C++: ループ直前に `#pragma omp parallel for reduction(+:correct)` を加える (正解数を数えるので reduction)。
- Fortran: ループを `!$omp parallel do private(...) reduction(+:correct)` と `!$omp end parallel do` で囲む。

各画像の推論で使う一時配列 (隠れ層 `h`) は各スレッドで別々に持つ必要がある (C++ はループ内で宣言済み, Fortran は `private` 指定)。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mnist_infer.cpp -o mnist_infer.exe

# Fortran
nvfortran -fast -mp=multicore mnist_infer.f90 -o mnist_infer.exe
```

```
OMP_NUM_THREADS=4 ./mnist_infer.exe
```

## 期待される結果

```
MNIST テスト 10000 枚: 正解 9776 枚, 正解率 = 97.76%
```

- 学習済みの重みを使うので, **本当に手書き数字を 9 割以上当てる**。並列化した行列積がそのまま認識器になっている。
- 各画像の計算は独立かつ決定的なので, **スレッド数を変えても正解率は同じ** (97.76%) になる。
- スレッド数を増やすと速くなる (「性能を比べる」セルで台数効果を測れる)。
- (発展) GPU にオフロードする, 行列積を SIMD 化する, などにも挑戦してみよ。
