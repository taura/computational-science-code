# 応用問題 (集大成): MLP に本物の MNIST を学習させる

## 目標

AI の「学習」の中身が実は **行列積の繰り返し** であることを, **多層パーセプトロン (MLP)** を一から実装して体験する。**forward (順伝播) → 損失 → backprop (逆伝播) → 勾配降下 (更新)** のループを自分で書き, **本物の MNIST 手書き数字**を分類できるネットワークを学習させる。

学習した重みは, 推論専用の問題 `00_mnist_infer` がそのまま読み込んで使う。**「学習」も「推論」も中身は同じ行列積**であることを確かめよう。

ネットワーク (入力 784 → 隠れ層 128 → 出力 10):

- forward: `h = ReLU(W1 x + b1)` (128次元),  `o = W2 h + b2` (10次元),  `p = softmax(o)`
- 損失: 多クラスのクロスエントロピー
- backprop: `do = p - onehot(y)`,  `gW2 += do·hᵀ`, `gb2 += do`, `dh = (W2ᵀ do)·[h>0]`, `gW1 += dh·xᵀ`, `gb1 += dh`
- 更新 (ミニバッチ勾配降下): バッチ内のサンプルにわたって勾配を**総和**し, `W -= lr·(勾配/バッチサイズ)`。

## 課題

データは NumPy 標準の **`.npy` 形式** (ヘッダ + 生バイナリ) で `data/` に用意してある。世の中で配布されている MNIST (`mnist.npz`) から取り出した訓練画像である:

- `data/x_train.npy` : 訓練画像 16000 枚 (各 784 画素, uint8 で 0..255)
- `data/y_train.npy` : 正解ラベル (int32, 0..9)

`.npy` の読み書き関数 `read_npy` / `write_npy` はソース内に用意済みなので, **入出力を書く必要はない** (並列化に集中せよ)。

各ミニバッチで一番重いのは **バッチ内の全サンプルにわたる forward + backprop の O(バッチ·HID·IN)**。各サンプルの寄与は独立なので並列化できる。ただし勾配配列 `gW1, gb1, gW2, gb2` への加算は競合するので, **配列 reduction** で安全に総和する:

- C++: `#pragma omp parallel for reduction(+:loss,correct,gb2[:OUT],gW2[:OUT*HID],gb1[:HID],gW1[:HID*IN])`
- Fortran: `!$omp parallel do private(...) reduction(+:loss,correct,gb2,gW2,gb1,gW1)` … `!$omp end parallel do` (サンプルごとの一時配列 `h,o,dout` は `private`)

これが `TODO` の並列化箇所である。スレッド数を変えても結果 (正解率) はほぼ同じになることを確認せよ。

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore mlp_train.cpp -o mlp_train.exe

# Fortran
nvfortran -fast -mp=multicore mlp_train.f90 -o mlp_train.exe
```

引数: エポック数 `E` (既定 20), 学習率 `lr` (既定 0.1), ミニバッチサイズ `BS` (既定 100)。

```
OMP_NUM_THREADS=4 ./mlp_train.exe 20 0.1 100
```

## 期待される結果

```
epoch    0: loss=1.16??, train acc=73.??%
epoch    5: loss=0.2???, train acc=92.??%
...
epoch   19: loss=0.1???, train acc=96.??%
最終: N=16000, HID=128, epochs=20, loss=0.1???, train acc=96.??%
elapsed = ... sec
重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました
```

- 学習が進むと **損失が下がり, 正解率が上がる**。終了時に学習済みの重みが `data/W1.npy` などに保存される。
- この重みを `00_mnist_infer` に渡すと, **未知のテスト画像を 9 割以上認識する** (汎化)。
- `OMP_NUM_THREADS` を増やすと `elapsed` が短くなる (台数効果)。正解率は本質的に同じになる。
- (発展) 内側の行列積を SIMD 化, あるいは GPU にオフロードして更に高速化できる。
