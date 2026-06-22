""" md
# データと重みの準備

学習済みの重み (`weights/`) とテスト画像 (`data/`) は Wisteria の共有領域に置いてある。
下を**一度だけ**実行して, 自分のフォルダにシンボリックリンクを張る。
これで `weights/{W1,b1,W2,b2}.npy` と `data/{x,y}_test.npy` が読めるようになる。
"""

""" codex w
%%bash_
ln -sfn /work/gt69/share/taura/data/mnist/weights
ln -sfn /work/gt69/share/taura/data/mnist/data
ls -l weights/ data/
"""
