""" md
# データの準備

学習に使う MNIST データ (`data/`) は Wisteria の共有領域に置いてある。下を**一度だけ**
実行して, データへのシンボリックリンク `data/` を張り, 学習した重みの出力先フォルダ
`weights/` を作る。これで `data/{x,y}_train.npy` を読み, 結果を `weights/{W1,b1,W2,b2}.npy`
に書き出せる (`weights/` は自分のフォルダ内。共有領域には書き込まない)。
"""

""" codex w
%%bash_
ln -sfn /work/gt69/share/taura/data/mnist/data
mkdir -p weights
ls -ld data/ weights/
"""
