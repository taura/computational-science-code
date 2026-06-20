""" md

""" include {out_dir}/{nn_topic}/problems/{nn_problem}/problem.md """

"""

""" md

# このノートについて

- この問題は「生成されたアセンブリ(機械語)を見て, SIMD命令に変換されたかを確認する」ことが目的。
- 関数だけのソース(`main` 無し)を `-c` でオブジェクトコンパイルし, `-Mkeepasm` で残る `.s` を読む。
- 実行はしない(`-c` なので実行ファイルは作らない)。

# ツールの読み込み

- AIチュータ及びジョブ投入ツールの読み込み (カーネル起動後に一度実行すればよい)
  - `heytutor` : `%%hey` でAIチュータに質問できるようになる (使い方は末尾を参照)

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
# Fortran ベースコード
"""

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

# AIチュータへの質問の仕方 (参考)

- 先頭で `import heytutor` 済みなら, セルに `%%hey` と書いて質問できる。
- ChatGPTなどと同様に自由に質問してよい。ただし「答えをそのまま教えて」などは自制すること。
- セル内で使える変数 (自動で展開される):
  - `{{file:FILENAME}}` : _FILENAME_ の中身 (例: `{{file:problem.md}}`, `{{file:{problem}.cpp}}`, `{{file:{problem}.s}}`)
  - `{{bash[-1]}}` : 最後に実行した `%%bash_` セルの入力・出力, `{{bash[-2]}}` = その前, ...
- 以下は質問例 (必要に応じてコピーして使う。Fortranなら `.cpp` を `.f90` に書き換える)。

## 一般的な質問

"""

""" codex w
%%hey

SIMD命令(packed double, vfmadd...pd など)って何?
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

- コンパイル時のエラー直後に実行するとエラーに関するヘルプが得られる。
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
## アセンブリについて質問
"""

""" codex w
%%hey

生成されたアセンブリを説明して。SIMD化されている?

ソース:
{{file:{problem}.cpp}}

アセンブリ:
{{file:{problem}.s}}
"""
