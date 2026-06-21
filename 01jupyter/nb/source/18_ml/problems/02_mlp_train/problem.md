# 応用問題 (集大成): MLP に本物の MNIST を学習させる

## 目標

AI の「学習」の中身が実は **行列積の繰り返し** であることを, **多層パーセプトロン (MLP)** を一から実装して体験する。**forward (順伝播) → 損失 → backprop (逆伝播) → 勾配降下 (更新)** のループを自分で書き, **本物の MNIST 手書き数字**を分類できるネットワークを学習させる。

学習した重みは, 推論専用の問題 `00_mnist_infer` がそのまま読み込んで使う。**「学習」も「推論」も中身は同じ行列積**であることを確かめよう。

ネットワーク (入力 784 → 隠れ層 128 → 出力 10)。ミニバッチ (`m` 枚) を**行列**として一度に流す (`X` は `m` 枚の入力をまとめた行列, 列が1サンプル):

- forward: `H = ReLU(W1 X + b1)`,  `P = softmax(W2 H + b2)`
- 損失: 多クラスのクロスエントロピー
- backprop: `dO = P - onehot(y)`,  `gW2 = dO Hᵀ`, `gb2 = Σ dO`, `dH = (W2ᵀ dO)·[H>0]`, `gW1 = dH Xᵀ`, `gb1 = Σ dH`
- 更新 (ミニバッチ勾配降下): `W -= lr·(勾配/バッチサイズ)`

## 課題

データは NumPy 標準の **`.npy` 形式** (ヘッダ + 生バイナリ) で `data/` に用意してある。世の中で配布されている MNIST (`mnist.npz`) から取り出した訓練画像である:

- `data/x_train.npy` : 訓練画像 16000 枚 (各 784 画素, uint8 で 0..255)
- `data/y_train.npy` : 正解ラベル (int32, 0..9)

パラメータ・バッチ・中間行列・勾配はすべて1つの構造体 (C++ `Net`, Fortran `net_t`) に固定サイズ配列でまとめてある。`.npy` の読み書き関数 (`read_npy` / `write_npy`) も用意済みなので, **入出力を書く必要はない** (並列化に集中せよ)。

**AI の計算の中身は「行列積」** である。各層の `行列積 + バイアス` を `matmul_bias`, 逆伝播の行列積を `matmul_back`, 重み勾配 (`gW2 = dO Hᵀ` など) を `grad_weight` という関数にまとめてある。これらが計算の主役 (重い) で, **行列積の独立な出力方向ループを `#pragma omp parallel for`(Fortran は `!$omp parallel do`)で並列化するだけ**でよい。サンプル(バッチ)方向の和は行列積の内側の縮約に入るので, 勾配への**配列 reduction は要らない**(損失・正解数のスカラ集計 `eval` だけ `reduction(+:loss,correct)` を使う)。

- `matmul_bias` (前向きの各層): 行 (サンプル) ごとに独立 → その行ループを並列化。
- `matmul_back` (逆伝播の行列積): 行ごとに独立。
- `grad_weight` (重み勾配): 出力ごとに独立 (バッチ和は内側)。
- `eval` (損失・正解数): スカラ `reduction`。

これら (と `forward`/`backward` が呼ぶ) の中のループ先頭が `TODO`。活性化 (`relu`, `softmax`, `out_grad`, `relu_mask`) は要素ごとの軽い処理なので逐次のまま (並列化不要)。スレッド数を変えても結果 (正解率) は同じになることを確認せよ。

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
epoch   19: loss=0.0???, train acc=97.??%
最終: N=16000, HID=128, epochs=20, loss=0.0???, train acc=97.??%
elapsed = ... sec
重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました
```

- 学習が進むと **損失が下がり, 正解率が上がる**。終了時に学習済みの重みが `data/W1.npy` などに保存される。
- この重みを `00_mnist_infer` に渡すと, **未知のテスト画像を 9 割以上認識する** (汎化)。
- `OMP_NUM_THREADS` を増やすと `elapsed` が短くなる (台数効果)。正解率は本質的に同じになる。
- (発展) 内側の行列積を SIMD 化, あるいは GPU にオフロードして更に高速化できる。
