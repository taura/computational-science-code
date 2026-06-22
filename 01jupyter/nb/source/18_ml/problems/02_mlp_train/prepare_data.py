# MNIST 訓練データを .npy 形式で data/ に書き出す (著者用, 学生には配布されない)。
# 世の中で配布されている mnist.npz から, 学習に使う一部を取り出して .npy にする。
#   data/x_train.npy : uint8  [NTRAIN, 784]  (28x28 画像を一列にしたもの, 画素 0..255)
#   data/y_train.npy : int32  [NTRAIN]       (正解ラベル 0..9)
# .npy は NumPy 標準の配布形式で, ヘッダ + 生バイナリ。mlp_train.cpp/f90 が直接読む。
# 01jupyter ディレクトリで python3 nb/source/18_ml/problems/02_mlp_train/prepare_data.py
import os
import numpy as np

NTRAIN = 16000           # 学習に使う枚数 (リポジトリを軽くするため一部のみ)
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, "data")
os.makedirs(out, exist_ok=True)

d = np.load(os.path.expanduser("~/.keras/datasets/mnist.npz"))
x = d["x_train"][:].reshape(-1, 784).astype(np.uint8)   # 画素は 0..255 のまま
y = d["y_train"][:].astype(np.int32)
np.save(os.path.join(out, "x_train.npy"), x)
np.save(os.path.join(out, "y_train.npy"), y)
print(f"wrote {out}/x_train.npy {x.shape} {x.dtype}, y_train.npy {y.shape} {y.dtype}")
