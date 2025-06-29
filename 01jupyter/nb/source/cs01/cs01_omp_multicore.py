""" md

#* 高性能プログラミングと性能測定(1) --- OpenMP CPUマルチコアプログラミング

# OpenMP

* OpenMPはCPUでマルチコア並列処理を行うための標準(デファクトスタンダード)
* 新しい仕様ではGPUもサポートしている(どこまでサポートしているかはコンパイラ依存)

* 詳しい仕様が知りたくなったら https://openmp.org/ を参照
  * 最新仕様 https://www.openmp.org/spec-html/5.2/openmp.html
  * 簡潔な文法のリファレンス: https://www.openmp.org/resources/refguides/
* 最小限の覚えるべきキーワード
  * `#pragma omp parallel`
  * `#pragma omp for`
  * `reduction`
* 最小限の覚えるべきAPI関数
```
#include <omp.h> 
```
して
  * omp_get_num_threads();
  * omp_get_thread_num();

# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと

## コンパイラ

* この演習環境では, 同じコンパイラでCPUもGPUもサポートしているという理由で, NVIDIA HPC SDKを使う
* コマンド名:
  * C: `nvc`
  * C++: `nvc++`
* コンパイルオプション:
  * `-mp=multicore` をつけると CPU用のOpenMPがサポートされる
  * `-mp=gpu` をつけると GPU用のOpenMPがサポートされる (次週)
* 上記のコマンドを実行できるようにするために, 以下を実行する
  * なお以下はコマンドライン端末上では `module load nvidia` とするのが本来のやり方だがJupyter上で`module`コマンドが動かないのでやむなく以下のようにする
"""

""" code w """
import os
paths = os.environ["PATH"].split(":")
nvc_path="/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin"
if nvc_path not in paths:
    os.environ["PATH"] = ":".join([nvc_path] + paths)
""" """

""" md

## ジョブ投入を簡便に行うスクリプト

"""

""" code """
import sys
submit_path = "/work/gt47/share/taura/computational-science-code/00submit"
if submit_path not in sys.path:
    sys.path.append(submit_path)
import submit
""" """


""" md
# `#pragma omp parallel` 構文

* OpenMPで, 「これがなくては始まらない」プリミティブ
* OpenMPプログラムは1スレッドで実行を開始し(MPIのようにmain関数が複数実行されるのではない), `#pragma omp parallel`に差し掛かると直後に書かれた文を複数のスレッドが実行する
* いくつできるかの規則や制御方法にはいくつかあるが, もっとも基本的なルールは, 実行時の環境変数OMP_NUM_THREADSで指定した数, というもの

* 以下を実行すると, `#pragma omp parallel`の直下に書かれた一文 `printf("in parallel\n");` が複数のスレッドによって実行される
"""

""" code w """
%%writefile omp_parallel.c
""" include nb/source/cs01/include/omp_parallel.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel.c -o omp_parallel_mp.exe
""" """

""" md
* `OMP_NUM_THREADS` という環境変数 (「環境変数」を知らない人は以下のようにコマンドの前にある`変数名=値`のことだと思えば良い) を設定すると, `printf("in parallel\n");` (`#pragma omp parallel` 直下の文) を実行するスレッドの数を変えられる
* 以下はログインノード上で実行する
* `OMP_NUM_THREADS`に設定する値を色々変えて実行してみよ
"""

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_mp.exe
""" """

""" md
* 以下はジョブ投入をして実行する
* 必要に応じて rscgrp や elapse などを指定して実行せよ
* 例:
```
#PJM -L rscgrp=lecture7-a
#PJM -L elapse=0:10:00
```
"""

""" code w """
%%bash_submit
OMP_NUM_THREADS=3 ./omp_parallel_mp.exe
""" """

""" md
* 以下では必要のない場合は `%%bash_submit` ではなく `%%bash` を用いた実行セルを示すが, 必要に応じて自分で `%%bash_submit` に変えて実行せよ

"""


""" md

* 文法
```
#pragma omp parallel
文
```
* 意味
 * 「文」を複数のスレッドで実行する(典型的には環境変数 `OMP_NUM_THREADS` で指定した数)
  * それらのスレッドを, その文を実行する<font color="blue">チーム</font>と呼ぶ
 * チームの全スレッドが「文」を実行し終えると, `#pragma omp parallel` 全体が実行を終える
 * 再び1スレッドに戻って以降の文を実行する
* 注
 * 複数のスレッドが実行するのは `#pragma omp parallel` の直下に書かれたひとつの文だが, この文自体が複合文 (`{ ... }` で囲まれた複数の文をまとめて一つの文とみなしたもの)だったり, ループだったり, 関数呼び出しを含んで, その関数の中に多数の文を含んでいることがあるので, 実際には複数スレッドで実行される文の数はいくらでも多数であり得る

# `#pragma omp parallel` を実行するスレッド数の指定の仕方色々

* プログラムの中で指定する
  * `#pragma omp parallel` に `num_threads(N)` として `N` 個のスレッドで実行するよう指定できる
  * `N` はCの式でよいのでこれをプログラムの引数で指定したり, プログラム中のデータに応じて調節したりということもやろうと思えばできる
  * ここで指定したものは環境変数 `OMP_NUM_THREADS` による指定よりも優先される
* 以下のプログラムの引数を変えて実行してみよ
"""

""" code w """
%%writefile omp_parallel_num_threads.c
""" include nb/source/cs01/include/omp_parallel_num_threads.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel_num_threads.c -o omp_parallel_num_threads_mp.exe
""" """

""" code w """
%%bash
./omp_parallel_num_threads_mp.exe 3
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

# omp_num_threads() と omp_thread_num()

*
```
#pragma omp parallel
  S
```
によってSを実行中のスレッドは, 
 * `omp_num_threads()` によってSを実行しているチームのスレッド数
 * `omp_thread_num()` によってその中での自分の番号(スレッド数を$n$として, 0以上$n$未満の数)
を得ることが出来る

* これらを使う場合

```
#include <omp.h>
```

しておく

"""

""" code w """
%%writefile omp_parallel_rank.c
""" include nb/source/cs01/include/omp_parallel_rank.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel_rank.c -o omp_parallel_rank_mp.exe
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

* `#pragma omp parallel`はその直下の文を「全員が(重複して)」実行する手段
* 実際に処理を高速化するには一定量の仕事を複数のスレッドで「分割」して処理する必要がある
* そのための手段が work sharing 構文. そのひとつが `#pragma omp for`

# `#pragma omp for` 構文

* `#pragma omp parallel`で指定された文を実行中にスレッドが `#pragma omp for` に到達すると, その直下に書かれた文 (for 文でなくてはならない)の繰り返しをスレッド間で分け合って実行する
* 仕事をスレッド間で分け合って実行するので, work sharing構文と呼ぶ 
* OpenMPには他のwork sharing構文もあるがこの演習ではそれらはやらない

* 以下は `#pragma omp for` の効果を確かめる簡単な例
"""

""" code w """
%%writefile omp_parallel_for.c
""" include nb/source/cs01/include/omp_parallel_for.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel_for.c -o omp_parallel_for_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=3 ./omp_parallel_for_mp.exe 10
""" """

""" md
* `m`回の繰り返しが複数のスレッドで分割されて実行されるため, 表示される順番が i の小さい順番とは限らなくなることに注意

* また, `#pragma omp for` を取り除くと, ループのすべての繰り返しがすべてのスレッドで実行されることに注意 (やってみよ)
* 普通はこんなプログラムは書かないが, 仕様を理解するための実験としてやってみよ

* OpenMPがやってくれることは基本的には for文の繰り返しを複数のスレッドで分割して実行するということである
* 逆に言うと, 計算時間の多くを並列に実行可能なfor文が占めていることが, OpenMPによる並列化が成功するための条件である

## `#pragma omp for` で実行できるfor文の制限

* 任意のfor文を並列実行できるわけではない
* 例えば break 文があるような, 途中で, 以降の繰り返しがすべて実行しないかもしれないようなfor文は並列実行できない(コンパイル時に文句を言われる)
* 大雑把には, 
```
#pragma omp for
for (変数 = 式1; 変数 < 式2; 変数 += 式3) {
  ...
}
```
の形をしており,
  * 式1, 式2, 式3はすべて, `#pragma omp for` に差し掛かった時点で値が確定し, ループ実行中に変化することがない
  * ... の途中でfor文を抜け出す文(break, returnなど)が存在しない
というもの
* 実際は `<` は `<=`, `>`, `>=` などでもよく, `+=` は `-=`, `++`, `--` でもよいなど細かい点で上記よりも柔軟
* 性質としては
  * for文に差し掛かった時点でループの繰り返し回数が判明する
  * 何回目の繰り返しで「変数」の値がいくらになるかが容易に計算可能(i 回目の繰り返しにおいて「変数 = 式1 + i * 式3」)
ということで, これによりスレッドに繰り返しを分割するのが容易になる

# `schedule` 句

* `#pragma omp for` では, 繰り返しをどのようにスレッドに割り当てるかを, `schedule` 句によって指定できる
* 繰り返し回数が$M$回, スレッド数が$T$個としたときに, 
  * `#pragma omp for schedule(static)` : はじめの$\lceil M/T \rceil$回をスレッド0, 次の$\lceil M/T \rceil$回をスレッド1, ... が実行 ($M$が$T$で割り切れなければ最後の方に少ないスレッドが出る場合も有る)
  * `#pragma omp for schedule(static, c)` : はじめの$c$回をスレッド0, 次の$c$回をスレッド1, ... が実行 (最後まで到達したらまたスレッド0に戻る)
  * `#pragma omp for schedule(dynamic)` : 各スレッドが, 「まだ実行されていない繰り返しを1つ実行する」を繰り返す. どの繰り返しをどのスレッドが実行することになるかは予測できない. 
  * `#pragma omp for schedule(dynamic, c)` : `dynamic`と同じだが, 繰り返しは一度につき$c$個ずつまとめてわりあてる. つまり「まだ実行されていない繰り返しを$c$個 (残った繰り返し数が$c$未満の場合はすべて) 実行する」を繰り返す. 
  * `#pragma omp for schedule(guided)` : `dynamic`と似ているが最初のうちは一度につき, 多めの繰り返しを割当て, 徐々に少なくしていく
  * `#pragma omp for schedule(guided, c)` : `guided`と似ているが最低でも$c$個割り当てる
  * `#pragma omp for schedule(runtime)` : 実行する際に環境変数 `OMP_SCHEDULE` で, 以下のようにschedule句を指定できる.
```
OMP_SCHEDULE=static コマンド
```  
* 何も書かなかった時のデフォルトが仕様で定められているかどうかは知らないが, 普通は初めの`static`の動作をする

* 以下のプログラムは, 各繰り返しを`usleep(i * 100000)`によって約$i \times 100$ミリ秒 ($=i \times 0.1) 秒かかるようにした上で, どの繰り返しをどのスレッドが実行したかを表示するもの
* `schedule`句を書き換えてコンパイル, 実行し, 上記を確認せよ
* どのようなときに`static` / `dynamic` を用いるべきか?

"""

""" code w """
%%writefile omp_parallel_for_thread_num.c
""" include nb/source/cs01/include/omp_parallel_for_thread_num.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel_for_thread_num.c -o omp_parallel_for_thread_num_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=2 time ./omp_parallel_for_thread_num_mp.exe 10
""" """

""" md
* 何度も書き換えてコンパイルするのが面倒になったら`schedule(runtime)` を使って, `OMP_SCHEDULE`をコマンドラインで指定するのが吉
"""

""" code w """
%%bash
OMP_NUM_THREADS=2 OMP_SCHEDULE=static time ./omp_parallel_for_thread_num_mp.exe 10
""" """

""" md

# `collapse` 句

* `#pragma omp for` は通常, その直下に書かれたfor文のみを並列(スレッド間で分割)実行の対象にする
* しかし, 並列化したいのが多重のfor文の入れ子(多重ループ)であることも多く, かつ当然ながらそういうfor文の方が計算量が多い傾向にある
* `#pragma omp for collapse(2)` のようにすることで直下に書かれた2重のfor文全体を並列実行の対象にできる (3重, 4重, ... も同様)
* 対象となるすべてのfor文に, 上記 (`#pragma omp for` で実行できるfor文の制限) で述べた制限がかかる

* 以下のプログラムのcollapseありとなしの結果を観察せよ
"""

""" code w """
%%writefile omp_parallel_for_collapse.c
""" include nb/source/cs01/include/omp_parallel_for_collapse.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_parallel_for_collapse.c -o omp_parallel_for_collapse_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=16 time ./omp_parallel_for_collapse_mp.exe 4
""" """



""" md

# 台数効果の目撃

* $T$個のスレッドを用いても速度が$T$倍(近く)にならないことはしばしば(「ほとんど」というべきか)
* そもそも$T > $コア数のときは複数のスレッドが同じコア上で動くことになり, コア数以降の向上は望めない(上のプログラムのようにスレッドがわざと(usleepで)休眠していたり, ファイルのIOなどで長時間止まっている場合は別)
* したがってOpenMPをそのような設定で使うことはあまりない
* $T \leq $コア数であっても, 速度が$T$倍 (実行時間が$1/T$倍) とは程遠くなる理由がいくつも有る
  1. 1つのスレッドによって実行される処理が小さすぎる ($T$個スレッドが実行を開始して, 全員が終了するのを待つ, というオーバーヘッドが目立つ)
  1. OSが速やかに$T$個のスレッドを別々のコアに割り当ててくれない
  1. スレッド間でデータを共有している場合にデータアクセスのコストが1スレッドの場合に比べて大きくなる(詳細は計算機の仕組みに関わるのでここでは深入りしない)
* 3番目はアルゴリズムの本質に根ざした問題で容易に除去できない場合が多いが,
  * 1についてはそういうものだと思っておく(あまり短すぎる処理を複数コアで性能向上はできない. あまりそうする意味もないのでやる必要もない)
  * 2については, 実行時に`OMP_PROC_BIND=true` と環境変数を設定することで改善することがある. `OMP_PROC_BIND=true` は, 各スレッドを特定のコアでしか実行しない, かつそれらが均等になるという効果を持つ指示で, 2の問題を緩和できる. 説明: https://www.openmp.org/spec-html/5.0/openmpse52.html 

* 以下では, 計算自身にあまり意味はないが簡単な例題で性能向上を目撃してみる
* なお `#pragma omp parallel for` は `#pragma omp parallel` の直後に `#pragma omp for` を書いたのと同じ効果を持つ
"""

""" code w """
%%writefile omp_speedup.c
""" include nb/source/cs01/include/omp_speedup.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_speedup.c -o omp_speedup_mp.exe
""" """

""" md
* 以下は, $m = 72$, $n = 100 \times 1000 \times 1000$として実行する
* スレッド数を変えて, 仮想コア数付近までの性能向上 (GFLOPS値の向上), それ以降の頭打ちを確認せよ
"""

""" code w """
%%bash
OMP_NUM_THREADS=1 ./omp_speedup_mp.exe 72 $((100 * 1000 * 1000))
""" """

""" md
* 手動でやるのが嫌になったら以下で一撃で実行
"""

""" code w """
%%bash
for x in 1 2 3 4 6 8 9 12 18 21 24 27 30 33 36; do
    echo -n "$x "
    OMP_NUM_THREADS=${x} OMP_PROC_BIND=true ./omp_speedup_mp.exe 72 $((100 * 1000 * 1000)) | grep GFLOPS
done
""" """

""" md
* Wisteria Aquarius の計算ノードは 72 コアで, 8 GPUを搭載しており, 1 GPUにつき 72 / 8 = 9 コアが割り当てられる
* 36 コアまで性能向上させたければ
```
#PJM -L gpu=4
#PJM --omp thread=36
```
を指定する必要がある
"""

""" code w """
%%bash_submit
#PJM -L gpu=4
#PJM --omp thread=36
#PJM -L elapse=0:05:00

for x in 1 2 3 4 6 8 9 12 18 21 24 27 30 33 36; do
    echo -n "$x "
    OMP_NUM_THREADS=${x} OMP_PROC_BIND=true ./omp_speedup_mp.exe 72 $((100 * 1000 * 1000)) | grep GFLOPS
done
""" """


""" md
* 結果を以下で可視化 (上の結果をコピペせよ)
"""

""" code w """
""" include nb/source/cs01/include/speedup.py """
""" """

""" md
* なおこの実験はいろいろな意味でいい加減な実験だということを注意しておく
  * 本来は同じ条件で何度も実験して, ばらつきなどを見つつ平均をとるべき
  * `OMP_PROC_BIND=true` を設定して複数のスレッドが同一コアに割り当たらないようにしているが, 同時に複数の人が実験すると, 同一コアに複数の人のスレッドが割り当てられることがありうるので実行したタイミングの運・不運で結果が変わる
  * 本来は一回のプログラム中でも同じループを複数回実行して計測するのが正しい
  * 特に何事も「プログラム中の最初の一回の◯◯」というのは特別な処理が必要になりがちで, 実際OpenMPでも初めて`#pragma omp parallel`に遭遇したときにOSのスレッドが生成され, それ以降は同じスレッドが使い回されるということが通常である(つまり初めての `#pragma omp parallel` はそれ以降より余分なオーバーヘッドがかかりやすい)
  * など  

# OpenMPにおけるデータの共有

* OpenMPのスレッドは基本的に全てのデータ(変数や配列)を<font color="blue">「共有」</font>している
* 「共有」しているとは, 大雑把にいえば, どのスレッドが変数に書き込んだ値も, 他のスレッドに見えるということである
* そのことはこれまでの例題プログラムでも暗黙的に前提としていたことで, 例えば以下で, 
```
#pragma omp parallel for
  for (long i = 0; i < m; i++) {
    x[i] = lin_rec(0.99, i + 1, 1.0, n);
  }
  /* 計測終了 */
  double t1 = omp_get_wtime();
  double dt = t1 - t0;          /* sec */
  /* 答え表示 (x[i] = 100 * (i + 1) くらいのはず) */
  for (long i = 0; i < m; i++) {
    printf("x[%3ld] = %9.3f\n", i, x[i]);
  }
```
各スレッドが`x[i]`に書き込んだ値が「答え表示」で表示できるのも, スレッドがデータを共有しているからである

* データが共有されているということは便利でもある一方で実は気をつけなくてはいけないことがある
* 以下では, データが共有されていることで生ずる問題<font color="red">競合状態</font>とその解消法について説明する

# 競合状態

* 以下のコードをOpenMPで並列化することを考える

"""

""" code w """
%%writefile integral.c
""" include nb/source/cs01/include/integral.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore integral.c -o integral_mp.exe
""" """

""" code w """
%%bash
./integral_mp.exe
""" """

""" md 

* ここにそのまま pragma parallel, pragma for を当てはめると以下のようになる

"""

""" code w """
%%writefile omp_integral_racy.c
""" include nb/source/cs01/include/omp_integral_racy.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_integral_racy.c -o omp_integral_racy_mp.exe
""" """

""" md

* 正解は $\pi/4$で 1スレッドで実行すれば問題なく正解が出る

"""

""" code w """
%%bash
OMP_NUM_THREADS=1 time ./omp_integral_racy_mp.exe
""" """

""" md

* 2スレッド以上だと正解は出ない上, 毎回答えも違う(非決定的な挙動)

"""

""" code w """
%%bash
OMP_NUM_THREADS=4 time ./omp_integral_racy_mp.exe
""" """

""" md

* その理由を考察する(講義スライド pXX 〜に答えがある)

<pre>
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    double x = a + i * dx;
    s += 1 / (1 + x * x);
  }
</pre>

において, 変数 sは全部のスレッドで共有されている(注: `x` は `#pragma omp parallel` で実行される文の内部で定義されており, そのような変数はスレッドごとに別の変数になる. つまり共有されない)
* 複数のスレッドが同じ変数`s`を更新することになる
* よくないことが起きる具体的なシナリオはスライドpXX を参照. `s += ...` は実際には, `s` を読み出し, `...` を足して, それをまた`s`に書き戻す. 読み出しから書き戻しまでの間に別のスレッドが `s` を更新するとおかしなことになる


* このように, 複数のスレッドが同じ変数を使っていて, 少なくとも一人は更新している状況を<font color="red">「競合状態」</font>と呼び, 大概のプログラムは意図した動作をしない
* 競合状態があったら必ず何か手を打たないといけないと思っておくべき

## reduction 節を用いた解決法

* 今回の変数 `s` に対する足し込みのような演算(もう少し一般的な話は後で述べる)に対してはとりわけ簡単な解決法
* `#omp pragma parallel` もしくは `#omp pragma for` の中に `reduction(演算:変数)` という節を付け加えれば良い

"""

""" code w """
%%writefile omp_integral_reduction.c
""" include nb/source/cs01/include/omp_integral_reduction.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_integral_reduction.c -o omp_integral_reduction_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=4 time ./omp_integral_reduction_mp.exe
""" """

""" md 

* reductionは一般に多数の値を縮約する演算 o ($s = s_0 {\rm o} s_1 {\rm o}  ... {\rm o} s_{n-1}$)であって, 適用する順番を変えても同じ答えが出る場合に使われる
* つまり足し算(+)であれば $s_0 + s_1 + s_2 + s_3$を, 
$$(((s_0 + s_1) + s_2) + s_3)$$
と計算しても良いし 
$$(s_0 + s_1) + (s_2 + s_3)$$
と計算しても良い. 
* 見ての通り後者のように計算すれば $(s_0 + s_1)$ と $(s_2 + s_3)$ を並行に計算できることになる
* したがって reduction は, 結合則と交換則が成り立つような演算を多数の要素に施す場合に使え, 代表例としては, +, *, max, min, などがある

* ただし実は, 浮動小数点数(`double`, `float`など)の足し算には丸め誤差が生ずる
* そのため浮動小数点数を多数足し合わせる場合, その順番により結果が変わりうる
* `reduction`を使った場合, 1スレッドの実行と答えがずれる, スレッド数によって答えが少し変わるということは普通に生ずる

# 終わりに一言

* これでOpenMPを用いて, マルチコア(複数のコアを使った)並列処理ができるようになった
* ただしこれだけでCPUの性能をどのくらい引き出せるかと言うと, 本来性能には程遠い
* その理由は主にSIMD命令を用いていないこと, 命令間の依存関係によって, 命令レベル並列を引き出せていないこと, である

"""
