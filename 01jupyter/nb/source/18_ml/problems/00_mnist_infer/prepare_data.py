# MNIST テスト画像を .npy 形式で data/ に書き出す (著者用, 学生には配布されない)。
# 世の中で配布されている mnist.npz から, テスト用の一部を取り出して .npy にする。
#   data/x_test.npy : uint8  [NTEST, 784]  (28x28 画像を一列にしたもの, 画素 0..255)
#   data/y_test.npy : int32  [NTEST]       (正解ラベル 0..9)
# .npy は NumPy 標準の配布形式で, ヘッダ + 生バイナリ。mnist_infer.cpp/f90 が直接読む。
#
# 認識に使う学習済みの重み (data/W1.npy, b1.npy, W2.npy, b2.npy) は別問題
# 02_mlp_train の mlp_train を実行して生成し, ここの data/ にコピーする:
#   (02_mlp_train で) ./mlp_train_cpp.exe   -> data/{W1,b1,W2,b2}.npy ができる
#   cp ../02_mlp_train/data/{W1,b1,W2,b2}.npy ./data/
#
# 01jupyter ディレクトリで python3 nb/source/18_ml/problems/00_mnist_infer/prepare_data.py
import os
import numpy as np

NTEST = 2000             # 認識させる枚数 (リポジトリを軽くするため一部のみ)
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, "data")
os.makedirs(out, exist_ok=True)

d = np.load(os.path.expanduser("~/.keras/datasets/mnist.npz"))
x = d["x_test"][:NTEST].reshape(NTEST, 784).astype(np.uint8)   # 画素は 0..255 のまま
y = d["y_test"][:NTEST].astype(np.int32)
np.save(os.path.join(out, "x_test.npy"), x)
np.save(os.path.join(out, "y_test.npy"), y)
print(f"wrote {out}/x_test.npy {x.shape} {x.dtype}, y_test.npy {y.shape} {y.dtype}")
