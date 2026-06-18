""" md

#* OpenMP入門(1) --- `parallel` 構文とスレッド番号

# OpenMP

* OpenMPはCPUでマルチコア並列処理を行うための標準(デファクトスタンダード)
* 新しい仕様ではGPUもサポートしている(どこまでサポートしているかはコンパイラ依存)
* C/C++ と Fortran の両方から使える
  * C/C++ では `#pragma omp ...` という指示行(ディレクティブ)で書く
  * Fortran では `!$omp ...` という形のコメント風の指示行で書く
  * どちらも「並列化の指示を、元の逐次プログラムにそっと付け加える」という発想は共通

* 詳しい仕様が知りたくなったら https://openmp.org/ を参照
  * 最新仕様 https://www.openmp.org/spec-html/5.2/openmp.html
  * 簡潔な文法のリファレンス: https://www.openmp.org/resources/refguides/

* このトピックで覚えるべきキーワード
  * `#pragma omp parallel` (Fortran: `!$omp parallel` ... `!$omp end parallel`)
* このトピックで覚えるべきAPI関数 (C/C++ では `#include <omp.h>`, Fortran では `use omp_lib`)
  * `omp_get_num_threads();`
  * `omp_get_thread_num();`

# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと

## コンパイラ

* Aquariusでは, 同じコンパイラでCPUもGPUもサポートしているという理由で, NVIDIA HPC SDKを使う
  * コマンド名:
    * C: `nvc`
    * C++: `nvc++`
    * Fortran: `nvfortran`
  * コンパイルオプション:
    * `-mp=multicore` をつけると CPU用のOpenMPがサポートされる
    * `-mp=gpu` をつけると GPU用のOpenMPがサポートされる
* Odysseyでは, 富士通コンパイラを使う
  * コマンド名:
    * C: `fccpx`
    * C++: `FCCpx`
    * Fortran: `frtpx`
  * コンパイルオプション:
    * `-Kopenmp` をつけると CPU用のOpenMPがサポートされる
* 上記のコマンドを実行できるようにするために, 以下を実行する
  * なお以下はコマンドライン端末上では `module load nvidia`, `module load fj` とするのが本来のやり方だがJupyter上で`module`コマンドが動かないのでやむなく以下のようにする
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

""" md
* 以下でコンパイラのパス名が無事表示されれば成功
"""

""" code w """
%%bash
which nvc++
which nvfortran
which FCCpx
""" """

""" md

## ジョブ投入を簡便に行う設定

"""

""" code """
import wisteria_submit
""" """

""" md

# AIチューター

"""

""" code """
import heytutor
""" """

""" md
# `parallel` 構文

* OpenMPで, 「これがなくては始まらない」プリミティブ
* OpenMPプログラムは1スレッドで実行を開始し(MPIのようにmain関数が複数実行されるのではない), `parallel`構文に差し掛かると直後に書かれた部分を複数のスレッドが実行する
* いくつできるかの規則や制御方法にはいくつかあるが, もっとも基本的なルールは, 実行時の環境変数`OMP_NUM_THREADS`で指定した数, というもの

* 文法
  * C/C++:
```
#pragma omp parallel
文
```
  * Fortran:
```
!$omp parallel
文の並び
!$omp end parallel
```
* 意味
  * 「文」(Fortranでは `!$omp parallel` から `!$omp end parallel` までの範囲)を複数のスレッドで実行する(典型的には環境変数 `OMP_NUM_THREADS` で指定した数)
  * それらのスレッドを, その部分を実行する<font color="blue">チーム</font>と呼ぶ
  * チームの全スレッドが実行を終えると, `parallel`構文全体が実行を終える
  * 再び1スレッドに戻って以降の文を実行する
* 注
  * C/C++で複数のスレッドが実行するのは `#pragma omp parallel` の直下に書かれたひとつの文だが, この文自体が複合文 (`{ ... }` で囲まれた複数の文をまとめて一つの文とみなしたもの)だったり, ループだったり, 関数呼び出しを含んで, その関数の中に多数の文を含んでいることがあるので, 実際には複数スレッドで実行される文の数はいくらでも多数であり得る
  * Fortranでは `!$omp parallel` ... `!$omp end parallel` で囲まれた範囲がそのまま複数スレッドの実行対象になる

* 以下を実行すると, `parallel`構文の直下に書かれた `printf("in parallel\n");` (Fortranでは `print` 文) が複数のスレッドによって実行される

## C++版
"""

""" code w """
%%writefile omp_parallel.cpp
""" include nb/source/02_parallel/include/omp_parallel.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel.cpp -o omp_parallel_mp.exe
""" """

""" md
## Fortran版
* 同じ内容をFortranで書くと以下のようになる
"""

""" code w """
%%writefile omp_parallel.f90
""" include nb/source/02_parallel/include/omp_parallel.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel.f90 -o omp_parallel_f_mp.exe
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp omp_parallel.cpp -o omp_parallel_mp.exe
frtpx -Kfast -Kopenmp omp_parallel.f90 -o omp_parallel_f_mp.exe
```
"""

""" md
* `OMP_NUM_THREADS` という環境変数 (「環境変数」を知らない人は以下のようにコマンドの前にある`変数名=値`のことだと思えば良い) を設定すると, `parallel`構文の直下を実行するスレッドの数を変えられる
* 以下はログインノード上で実行する
* `OMP_NUM_THREADS`に設定する値を色々変えて実行してみよ
"""

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_f_mp.exe
""" """

""" md
* 以下はジョブ投入をして実行する
* 必要に応じて rscgrp や elapse などを指定して実行せよ
* 例:
```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00
```
"""

""" code w """
%%bash_submit
OMP_NUM_THREADS=3 ./omp_parallel_mp.exe
""" """

""" md
* 以下は富士通コンパイラでコンパイルした Odyssey 用の実行
"""

""" code w """
%%bash_submit_o
OMP_NUM_THREADS=3 ./omp_parallel_mp.exe
""" """

""" md
* 以下では必要のない場合は `%%bash_submit` ではなく `%%bash` を用いた実行セルを示すが, 必要に応じて自分で `%%bash_submit` に変えて実行せよ
"""

""" md
# `parallel` 構文を実行するスレッド数の指定の仕方色々

* プログラムの中で指定する
  * `parallel`構文に `num_threads(N)` として `N` 個のスレッドで実行するよう指定できる
  * `N` は式でよいのでこれをプログラムの引数で指定したり, プログラム中のデータに応じて調節したりということもやろうと思えばできる
  * ここで指定したものは環境変数 `OMP_NUM_THREADS` による指定よりも優先される
* 以下のプログラムの引数を変えて実行してみよ

## C++版
"""

""" code w """
%%writefile omp_parallel_num_threads.cpp
""" include nb/source/02_parallel/include/omp_parallel_num_threads.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel_num_threads.cpp -o omp_parallel_num_threads_mp.exe
""" """

""" code w """
%%bash
./omp_parallel_num_threads_mp.exe 3
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_parallel_num_threads.f90
""" include nb/source/02_parallel/include/omp_parallel_num_threads.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel_num_threads.f90 -o omp_parallel_num_threads_f_mp.exe
""" """

""" code w """
%%bash
./omp_parallel_num_threads_f_mp.exe 3
""" """

""" md
* 環境変数 `OMP_NUM_THREADS` で指定する (推奨)
  * 上述したとおり
  * 手軽にスレッド数を変更できるので多くの場合はこれを用いる
  * 一般に, プログラムはスレッド数がいくつであっても実行するように書いておき, 実行するときに調節できるようにするのが良いので, その意味でも特に理由がなければこれを使うことを推奨

* 指定しない
  * どちらも指定しなければ (仕様にそう書かれているかは知らないが普通), 環境に備わる仮想コア数を `OMP_NUM_THREADS` に指定したのと同じ効果になる
  * 「仮想コア」については後述
  * 以下のコマンドでそれを知れる
"""

""" code w """
%%bash
nproc
""" """

""" md
* `omp_parallel` を `OMP_NUM_THREADS` 指定せずに実行してみよ
"""

""" code w """
%%bash
./omp_parallel_mp.exe
""" """

""" md
* `OMP_NUM_THREADS` には自由な数を指定してよいが, CPUに搭載されている仮想コア数より大きくしても計算が速くなることはまずない

# `omp_get_num_threads()` と `omp_get_thread_num()`

* `parallel`構文の中身 S を実行中のスレッドは,
  * `omp_get_num_threads()` によってSを実行しているチームのスレッド数
  * `omp_get_thread_num()` によってその中での自分の番号(スレッド数を$n$として, 0以上$n$未満の数)
  を得ることが出来る

* これらを使う場合
  * C/C++: `#include <omp.h>`
  * Fortran: `use omp_lib`
  しておく

## C++版
"""

""" code w """
%%writefile omp_parallel_rank.cpp
""" include nb/source/02_parallel/include/omp_parallel_rank.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel_rank.cpp -o omp_parallel_rank_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_rank_mp.exe
""" """

""" code w """
%%bash
./omp_parallel_rank_mp.exe
""" """

""" md
## Fortran版
* Fortranでは, 各スレッドごとに別の値を持つべき変数 (`omp_nthreads`, `omp_rank`) を `private` 句で「スレッドごとに別の変数にする」と指定している点に注意
  * これを忘れると全スレッドが同じ変数を共有してしまい, 表示がおかしくなる
  * C/C++版で `parallel`構文の `{ ... }` の中で変数を宣言したのと同じ効果をFortranでは `private` 句で実現している
  * (データの共有・非共有の詳しい話は後のトピックで扱う)
"""

""" code w """
%%writefile omp_parallel_rank.f90
""" include nb/source/02_parallel/include/omp_parallel_rank.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel_rank.f90 -o omp_parallel_rank_f_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_rank_f_mp.exe
""" """

""" md

# CPU, コア, 仮想コア(ハードウェアスレッド)

* 最近の<font color="blue">CPU</font>はマルチコアCPUで, 1つのチップに複数の, 独立に実行可能な<font color="blue">「コア」</font>が搭載されている
* ひとつのボード(計算ノード)に複数のCPUが搭載されていることもある(<font color="blue">マルチソケット</font>環境)
* さらにひとつのコアにも独立に命令を実行可能な, <font color="blue">「仮想コア(ハードウェアスレッド)」</font>が搭載されている
* 田浦 <a href="https://taura.github.io/cs-alliance/slides/pdf/taura_lecture.pdf" target="_blank">講義スライド</a> pXXを参照
* ソフトウェア(OS)からは仮想コアがひとつのプロセッサとして見える
* ただしハードウェアスレッドは起動時にOFFにすることもでき, その場合は観測されるプロセッサ数は, 物理コア数になる
* OSから見えているプロセッサ数は以下のコマンドで観測できる
"""

""" code w """
%%bash
nproc
""" """

""" md
* CPUの機種名は以下で観測できる
"""

""" code """
%%bash
cat /proc/cpuinfo | head -30
""" """

""" md

* コアを, 仮想コアと区別して「物理コア」と呼ぶこともある
* 物理コアとか仮想コアの違いはソフトウェアからはほとんど見えないが, 同一の物理コア上の仮想コアは演算器を共有しており, 「一サイクルに実行可能な浮動小数点演算数」みたいな数字は「コア」あたりの数字である. すでに限界性能に近いスレッドを二つ, 同一のコアに置いても性能は倍にならない

* `parallel`構文はその直下を「全員が(重複して)」実行する手段
* 実際に処理を高速化するには一定量の仕事を複数のスレッドで「分割」して処理する必要がある
* そのための手段が work sharing 構文 (`#pragma omp for` など) で, 次のトピックで扱う
"""
