""" md

""" include {out_dir}/{nn_topic}/problems/{nn_problem}/problem.md """

"""

""" md

# ツールの読み込み

- AIチュータ及びジョブ投入ツールの読み込み (カーネル起動後に一度実行すればよい)
  - `heytutor` : `%%hey` でAIチュータに質問できるようになる (使い方は末尾を参照)
  - `wisteria_submit` : `%%bash_submit` (先頭に `#PJM ...` を書く) でジョブ投入できるようになる

"""

""" code w """
import heytutor
import wisteria_submit
""" """

""" md
# C++ ベースコード
"""

""" code w """
%%writefile_ {problem}.cpp
""" include {out_dir}/{nn_topic}/problems/{nn_problem}/{problem}.cpp """
""" """

""" md
## コンパイル
"""

""" codex w
%%bash_
PATH=/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin:/opt/FJSVxtclanga/tcsds-1.2.41/bin:$PATH
nvc++ -fast {mpflag} {problem}.cpp -o {problem}_cpp.exe
"""

""" md
## 実行

- 計算ノードにジョブとして投入して実行する。スレッド数・キュー・制限時間は `#PJM` 行で調整する。
- すぐにログインノードで試したいときは, 先頭の `%%bash_submit` を `%%bash_` に書き換えて (`#PJM` 行はコメントなので無視される) 実行すればよい。ただし数秒で終わる軽いジョブに限る。
"""

""" codex w
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:01:00
#PJM -L gpu=1
#PJM --omp thread=9
#PJM -g gt69
#PJM -j
#PJM -o 0output.txt

./{problem}_cpp.exe
"""

""" md
# Fortran ベースコード
"""

""" codex w
%%writefile_ {problem}.f90
""" include {out_dir}/{nn_topic}/problems/{nn_problem}/{problem}.f90 """
"""

""" md
## コンパイル
"""

""" codex w
%%bash_
PATH=/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin:/opt/FJSVxtclanga/tcsds-1.2.41/bin:$PATH
nvfortran -fast {mpflag} {problem}.f90 -o {problem}_f90.exe
"""

""" md
## 実行

- 計算ノードにジョブとして投入して実行する。スレッド数・キュー・制限時間は `#PJM` 行で調整する。
- すぐにログインノードで試したいときは, 先頭の `%%bash_submit` を `%%bash_` に書き換えて (`#PJM` 行はコメントなので無視される) 実行すればよい。ただし数秒で終わる軽いジョブに限る。
"""

""" codex w
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:01:00
#PJM -L gpu=1
#PJM --omp thread=9
#PJM -g gt69
#PJM -j
#PJM -o 0output.txt

./{problem}_f90.exe
"""

""" md

# 発展目標 (できる範囲で挑戦)

- この問題の基本は **マルチコア並列化** (`#pragma omp parallel for` / `!$omp parallel do` など)。まずはここまでを目指す。
- 余力があれば次にも挑戦してみよう (全部必須ではない):
  - **GPUで並列化**: コンパイルを `-mp=gpu` にして, 重いループに `#pragma omp target teams distribute parallel for` (+ 必要に応じて `map`) を付ける。
  - **SIMD化** (11/12章): 内側ループに `#pragma omp simd`, またはベクトル型。 **ILP向上** (13章): ベクトル長 `nl` の調整。
- どこまで速くできるか挑戦し, その効果を下の「性能を比べる」で可視化しよう。

# 性能を比べる (任意)

- 各プログラムは主計算部分の所要時間を `elapsed = ... sec` の形で表示する。
- 構成を変えて (スレッド数, SIMDの有無, GPU など) 実行し, 得られた **経過秒** を下の `DATA` に「ラベルと秒」で並べて実行すると, 棒グラフと「基準(先頭)比のスピードアップ」が出る。
- 試した構成だけ書けばよい (`#` で始まる行は無視)。

"""

""" code w """
import matplotlib.pyplot as plt

# 各構成の (ラベル, 経過秒) を並べる。先頭の行を基準(スピードアップ=1)にする。
# 自分が実際に試した構成の数値に書き換える。
DATA = [
    ("serial",    1.00),
    ("omp-8",     0.20),
    # ("simd",      0.50),
    # ("simd+omp",  0.07),
    # ("gpu",       0.05),
]

labels = [d[0] for d in DATA]
secs   = [float(d[1]) for d in DATA]
speed  = [secs[0] / s for s in secs]                 # 先頭(基準)比のスピードアップ
fig, ax = plt.subplots(1, 2, figsize=(9, 3))
ax[0].bar(labels, secs)
ax[0].set_ylabel("elapsed (s)")
ax[0].set_title("time (lower = faster)")
ax[1].bar(labels, speed)
ax[1].set_ylabel("speedup vs " + labels[0])
ax[1].set_title("speedup (higher = faster)")
for a in ax:
    a.tick_params(axis="x", rotation=30)
plt.tight_layout()
plt.show()
""" """

""" md

# AIチュータへの質問の仕方 (参考)

- 先頭で `import heytutor` 済みなら, セルに `%%hey` と書いて質問できる。
- ChatGPTなどと同様に自由に質問してよい。ただし「答えをそのまま教えて」などは自制すること。
- セル内で使える変数 (自動で展開される):
  - `{{file:FILENAME}}` : _FILENAME_ の中身 (例: `{{file:problem.md}}`, `{{file:{problem}.cpp}}`)
  - `{{bash[-1]}}` : 最後に実行した `%%bash_` セルの入力・出力, `{{bash[-2]}}` = その前, ...
- 以下は質問例 (必要に応じてコピーして使う。Fortranなら `.cpp` を `.f90` に書き換える)。

## 一般的な質問

"""

""" codex w
%%hey

C++の関数定義の文法どんなだっけ?
"""

""" md
## この問題に関するヒント
"""

""" codex w
%%hey

この問題に関するヒントを教えて

問題:
{{file:problem.md}}
"""

""" md
## 困ったときのヘルプ

- コンパイル時や実行時のエラー直後に実行するとエラーに関するヘルプが得られる。
"""

""" codex w
%%hey

以下のエラーが出た。何が間違い?

プログラム:
{{file:{problem}.cpp}}

コマンドと実行結果:
{{bash[-1]}}
"""

""" md
## フィードバック

- 答えが出た後も, 無駄なところやより良いやり方がないかを聞くことを推奨。
"""

""" codex w
%%hey

私の答に対するフィードバックをください。

問題:
{{file:problem.md}}

私の答:
{{file:{problem}.cpp}}
"""
