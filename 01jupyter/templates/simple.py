""" md 

""" include {out_dir}/{nn_topic}/problems/{nn_problem}/problem.md """

"""

""" md

# AIチューター

- 以下は必要に応じて実行（毎度実行する必要はない）

"""

""" code w """
import heytutor
""" """

""" md

## 一般的な質問

- ChatGPTなどに聞くときのように自由に質問可能。
- ただし「答えを教えて」などは自制すること。

"""

""" codex w
%%hey

C++の関数定義の文法どんなだっけ?
"""

""" md
## この問題に関するヒント

- `{{file:problem.md}}` は上記の問題文に展開される。

"""

""" codex w
%%hey

この問題に関するヒントを教えて

問題:
{{file:problem.md}}
"""

""" md

## いくつかの変数

* それぞれ以下のように展開される。

* `{{file:FILENAME}}` : _FILENAME_ の中身
* `{{bash[-1]}}` : 最後に実行した `%%bash_` セルの入力・出力, `{{bash[-2]}}` = その前の入力・出力, etc.

## 困ったときのヘルプ

* コンパイル時や実行時のエラー直後に以下を実行するとエラーに関するヘルプが得られる。

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

* 答えが出た後も、無駄なところや、より良いやり方がないかを聞くことを推奨。
* 以下のファイル名は適宜書き換えよ (Fortranなら `.cpp` -> `.f90` とするなど)

"""

""" codex w
%%hey

フィードバックを下さい。

問題:
{{file:problem.md}}

私の答:
{{file:{problem}.cpp}}
"""

""" md

# ジョブ投入ツール

- 以下を実行しておくと、`%%bash_submit_a` (Aquariousへのジョブ投入), `%%bash_submit_o` (Odyssey へのジョブ投入) が使えるようになる

"""

""" code w """
import wisteria_submit
""" """

""" md
# C++ ベースコード
"""

""" code w """
import heytutor
""" """

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
nvc++ -fast {problem}.cpp -o {problem}_cpp.exe
"""

""" md
## Run

- ログインノードでそのまま実行 (数秒で終わるジョブ)
"""

""" codex w
%%bash_
./{problem}_cpp.exe
"""

""" md
- Aquariusに投入
"""

""" codex w
%%bash_submit_a

./{problem}_cpp.exe
"""

""" md
- 上記は以下と同値
- キューや制限時間などを変更したいときは適宜変更・追加
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
## 質問/フィードバック
"""

""" codex w
%%hey

私の答に対するフィードバックをください。

問題:
{{file:problem.md}}

私の答:
{{file:{problem}.cpp}}

"""

""" md
# Fortran ベースコード
"""

""" code w """
import heytutor
""" """

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
nvfortran -fast {problem}.f90 -o {problem}_f90.exe
"""

""" md
## Run

- ログインノードでそのまま実行 (数秒で終わるジョブ)
"""

""" codex w
%%bash_
./{problem}_f90.exe
"""

""" md
- Aquariusに投入
"""

""" codex w
%%bash_submit_a
./{problem}_f90.exe
"""

""" md
- 上記は以下と同値
- キューや制限時間などを変更したいときは適宜変更・追加
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
## 質問/フィードバック
"""

""" codex w
%%hey

私の答に対するフィードバックをください。

問題:
{{file:problem.md}}

私の答:
{{file:{problem}.f90}}
"""

