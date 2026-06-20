""" md

""" include {out_dir}/{nn_topic}/problems/{nn_problem}/problem.md """

"""

""" md

# このノートについて

- この問題は「生成されたアセンブリ(機械語)を見て, SIMD命令に変換されたかを確認する」ことが目的。
- 関数だけのソース(`main` 無し)を `-c` でオブジェクトコンパイルし, `-Mkeepasm` で残る `.s` を読む。
- 実行はしない(`-c` なので実行ファイルは作らない)。

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

SIMD命令(packed double, vfmadd...pd など)って何?
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

* コンパイル時のエラー直後に以下を実行するとエラーに関するヘルプが得られる。

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
# C++ ベースコード
"""

""" code w """
import heytutor
""" """

""" codex w
%%writefile_ {problem}.cpp
""" include {out_dir}/{nn_topic}/problems/{nn_problem}/{problem}.cpp """
"""

""" md
## コンパイル (アセンブリを残す)

- `-c` : 実行ファイルを作らずオブジェクトファイルまで (`main` が無くてよい)
- `-Mkeepasm` : 生成アセンブリ `{problem}.s` を残す
- `-Minfo` / `-Mneginfo` : ベクトル化に成功/失敗したことを報告する
"""

""" codex w
%%bash_
PATH=/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin:/opt/FJSVxtclanga/tcsds-1.2.41/bin:$PATH
nvc++ -fast {mpflag} -Mkeepasm -Minfo -Mneginfo -c {problem}.cpp
"""

""" md
## 生成されたアセンブリを見る

- `pd` の付いた packed 命令 (`vmulpd`, `vaddpd`, `vfmadd...pd`) や, `%ymm`/`%zmm` レジスタが出ていればSIMD化されている。
"""

""" codex w
%%bash_
cat {problem}.s
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
## コンパイル (アセンブリを残す)
"""

""" codex w
%%bash_
PATH=/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin:/opt/FJSVxtclanga/tcsds-1.2.41/bin:$PATH
nvfortran -fast {mpflag} -Mkeepasm -Minfo -Mneginfo -c {problem}.f90
"""

""" md
## 生成されたアセンブリを見る
"""

""" codex w
%%bash_
cat {problem}.s
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
