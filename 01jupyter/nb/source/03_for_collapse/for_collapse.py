""" md

#* OpenMP入門(2) --- `for` 構文と `collapse` 句

# おさらい

* 前のトピックで学んだ `parallel` 構文は, その直下を「全員が(重複して)」実行する手段だった
* 実際に処理を高速化するには, 一定量の仕事を複数のスレッドで「分割」して処理する必要がある
* そのための手段が <font color="blue">work sharing 構文</font>. そのひとつが `#pragma omp for` (Fortranでは `!$omp do`)
* このトピックで覚えるべきキーワード
  * `#pragma omp for` (Fortran: `!$omp do` ... `!$omp end do`)
  * `collapse` 句

# 環境設定

* 前のトピックと同様, コンパイラとジョブ投入の設定を行う
* カーネルを再スタートしたときなどは失われるのでそのたびに行うこと
"""

""" code w """
import os
paths = os.environ["PATH"].split(":")
nvc_path = "/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin"
fj_path = "/opt/FJSVxtclanga/tcsds-1.2.41/bin"
for path in [nvc_path, fj_path]:
    if path not in paths:
        paths = [path] + paths
os.environ["PATH"] = ":".join(paths)
""" """

""" code w """
%%bash
which nvc++
which nvfortran
""" """

""" code """
import wisteria_submit
""" """

""" code """
import heytutor
""" """

""" md
# `for` 構文

* `parallel` 構文で指定された部分を実行中にスレッドが `#pragma omp for` (Fortran: `!$omp do`) に到達すると, その直下に書かれたループの繰り返しをスレッド間で分け合って実行する
* 仕事をスレッド間で分け合って実行するので, work sharing 構文と呼ぶ
* OpenMPには他のwork sharing構文もあるがこの演習ではそれらはやらない

* 文法
  * C/C++:
```
#pragma omp for
for (変数 = 式1; 変数 < 式2; 変数 += 式3) {
  ...
}
```
  * Fortran:
```
!$omp do
do 変数 = 式1, 式2, 式3
  ...
end do
!$omp end do
```

* 以下は `for` 構文の効果を確かめる簡単な例
* `parallel` の直下にループ以外の `printf` (`print`) があることに注意. これらは(work sharingされないので)全スレッドが実行する

## C++版
"""

""" code w """
%%writefile omp_parallel_for.cpp
""" include nb/source/03_for_collapse/include/omp_parallel_for.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel_for.cpp -o omp_parallel_for_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_for_mp.exe 10
""" """

""" md
## Fortran版
* C/C++ の `#pragma omp for` に当たるものが `!$omp do` ... `!$omp end do`
"""

""" code w """
%%writefile omp_parallel_for.f90
""" include nb/source/03_for_collapse/include/omp_parallel_for.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel_for.f90 -o omp_parallel_for_f_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_for_f_mp.exe 10
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp omp_parallel_for.cpp -o omp_parallel_for_mp.exe
frtpx -Kfast -Kopenmp omp_parallel_for.f90 -o omp_parallel_for_f_mp.exe
```
"""

""" md
* `m`回の繰り返しが複数のスレッドで分割されて実行されるため, 表示される順番が i の小さい順番とは限らなくなることに注意
* また, `#pragma omp for` (`!$omp do`) を取り除くと, ループのすべての繰り返しがすべてのスレッドで実行されることに注意 (やってみよ)
* 普通はこんなプログラムは書かないが, 仕様を理解するための実験としてやってみよ

* OpenMPがやってくれることは基本的にはループの繰り返しを複数のスレッドで分割して実行するということである
* 逆に言うと, 計算時間の多くを並列に実行可能なループが占めていることが, OpenMPによる並列化が成功するための条件である

## `for` 構文で実行できるループの制限

* 任意のループを並列実行できるわけではない
* 例えば break 文(Fortranでは `exit`)があるような, 途中で, 以降の繰り返しがすべて実行しないかもしれないようなループは並列実行できない(コンパイル時に文句を言われる)
* 大雑把には, C/C++では
```
#pragma omp for
for (変数 = 式1; 変数 < 式2; 変数 += 式3) {
  ...
}
```
の形をしており,
  * 式1, 式2, 式3はすべて, `#pragma omp for` に差し掛かった時点で値が確定し, ループ実行中に変化することがない
  * ... の途中でループを抜け出す文(break, returnなど)が存在しない
  というもの
* 実際は `<` は `<=`, `>`, `>=` などでもよく, `+=` は `-=`, `++`, `--` でもよいなど細かい点で上記よりも柔軟
* Fortranの `do 変数 = 初期値, 終了値, 増分` は, もともとこの形(繰り返し回数が事前に確定する)になっているので相性が良い. ただし `do while` は対象外
* 性質としては
  * ループに差し掛かった時点で繰り返し回数が判明する
  * 何回目の繰り返しで「変数」の値がいくらになるかが容易に計算可能
  ということで, これによりスレッドに繰り返しを分割するのが容易になる

# `collapse` 句

* `#pragma omp for` (`!$omp do`) は通常, その直下に書かれたループのみを並列(スレッド間で分割)実行の対象にする
* しかし, 並列化したいのが多重ループ(ループの入れ子)であることも多く, かつ当然ながらそういうループの方が計算量が多い傾向にある
* `collapse(2)` のようにすることで直下に書かれた2重ループ全体を並列実行の対象にできる (3重, 4重, ... も同様)
  * C/C++: `#pragma omp for collapse(2)`
  * Fortran: `!$omp do collapse(2)`
* 対象となるすべてのループに, 上記 (`for` 構文で実行できるループの制限) で述べた制限がかかる

* 以下のプログラムの collapse ありとなしの結果を観察せよ
* 各繰り返しを `usleep((i+j)*100000)` で約 $(i+j)\times100$ ミリ秒かかるようにした上で, どの繰り返しをどのスレッドが実行したかを表示する

## C++版
"""

""" code w """
%%writefile omp_parallel_for_collapse.cpp
""" include nb/source/03_for_collapse/include/omp_parallel_for_collapse.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel_for_collapse.cpp -o omp_parallel_for_collapse_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=16 time ./omp_parallel_for_collapse_mp.exe 4
""" """

""" md
## Fortran版
* Fortranには C の `usleep` に当たる標準の手続きがないので, `iso_c_binding` を使ってC言語の `usleep` を直接呼び出している
* `collapse(2)` を `!$omp do` だけ(collapseなし)に書き換えて, 違いを観察せよ
"""

""" code w """
%%writefile omp_parallel_for_collapse.f90
""" include nb/source/03_for_collapse/include/omp_parallel_for_collapse.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel_for_collapse.f90 -o omp_parallel_for_collapse_f_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=16 time ./omp_parallel_for_collapse_f_mp.exe 4
""" """

""" md
* collapse なしの場合, 外側ループ(`i`)の繰り返しだけがスレッドに分配される. つまり最大でも `m` 個のスレッドにしか仕事が行き渡らない
* collapse(2) の場合, `i`, `j` の組み合わせ全体(`m * m` 通り)がスレッドに分配されるので, より多くのスレッドに仕事を行き渡らせられる
* 多重ループで, 外側ループの繰り返し回数がスレッド数に比べて少ないようなときに `collapse` が効く

# `parallel` と `for` をまとめて書く

* `#pragma omp parallel` の直後に `#pragma omp for` を書く(あいだに他の文がない)という形は非常によくある
* この場合, 以下のようにまとめて書ける
  * C/C++: `#pragma omp parallel for`
  * Fortran: `!$omp parallel do` ... `!$omp end parallel do`
* 意味は `parallel` の直下に `for` をひとつ書いたのと同じ
* 以降のトピックではこの省略形もしばしば用いる
"""
