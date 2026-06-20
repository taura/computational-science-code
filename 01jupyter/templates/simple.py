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
