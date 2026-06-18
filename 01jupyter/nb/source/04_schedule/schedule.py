""" md

#* OpenMP入門(3) --- `schedule` 句

# おさらい

* 前のトピックで, `#pragma omp for` (Fortran: `!$omp do`) によってループの繰り返しをスレッド間で分け合って実行できることを学んだ
* このトピックでは, 「どの繰り返しをどのスレッドに割り当てるか」を制御する `schedule` 句を学ぶ
* このトピックで覚えるべきキーワード
  * `schedule(static)`, `schedule(dynamic)`, `schedule(guided)`, `schedule(runtime)`

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
# `schedule` 句

* `#pragma omp for` (Fortran: `!$omp do`) では, 繰り返しをどのようにスレッドに割り当てるかを, `schedule` 句によって指定できる
* 繰り返し回数が$M$回, スレッド数が$T$個としたときに,
  * `schedule(static)` : はじめの$\lceil M/T \rceil$回をスレッド0, 次の$\lceil M/T \rceil$回をスレッド1, ... が実行 ($M$が$T$で割り切れなければ最後の方に少ないスレッドが出る場合も有る)
  * `schedule(static, c)` : はじめの$c$回をスレッド0, 次の$c$回をスレッド1, ... が実行 (最後まで到達したらまたスレッド0に戻る)
  * `schedule(dynamic)` : 各スレッドが, 「まだ実行されていない繰り返しを1つ実行する」を繰り返す. どの繰り返しをどのスレッドが実行することになるかは予測できない.
  * `schedule(dynamic, c)` : `dynamic`と同じだが, 繰り返しは一度につき$c$個ずつまとめてわりあてる. つまり「まだ実行されていない繰り返しを$c$個 (残った繰り返し数が$c$未満の場合はすべて) 実行する」を繰り返す.
  * `schedule(guided)` : `dynamic`と似ているが最初のうちは一度につき, 多めの繰り返しを割当て, 徐々に少なくしていく
  * `schedule(guided, c)` : `guided`と似ているが最低でも$c$個割り当てる
  * `schedule(runtime)` : 実行する際に環境変数 `OMP_SCHEDULE` で, 以下のようにschedule句を指定できる.
```
OMP_SCHEDULE=static コマンド
```
* 何も書かなかった時のデフォルトが仕様で定められているかどうかは知らないが, 普通は初めの`static`の動作をする
* 書き方は C/C++ も Fortran も同じ (`#pragma omp for schedule(...)` / `!$omp do schedule(...)`)

* 以下のプログラムは, 各繰り返しを`usleep(i * 100000)`によって約$i \times 100$ミリ秒 ($=i \times 0.1$秒) かかるようにした上で, どの繰り返しをどのスレッドが実行したかを表示するもの
* `schedule`句を書き換えてコンパイル, 実行し, 上記を確認せよ
* どのようなときに`static` / `dynamic` を用いるべきか?

## C++版
"""

""" code w """
%%writefile omp_parallel_for_thread_num.cpp
""" include nb/source/04_schedule/include/omp_parallel_for_thread_num.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_parallel_for_thread_num.cpp -o omp_parallel_for_thread_num_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=2 time ./omp_parallel_for_thread_num_mp.exe 10
""" """

""" md
## Fortran版
* Fortranには C の `usleep` に当たる標準の手続きがないので, `iso_c_binding` を使ってC言語の `usleep` を直接呼び出している
"""

""" code w """
%%writefile omp_parallel_for_thread_num.f90
""" include nb/source/04_schedule/include/omp_parallel_for_thread_num.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_parallel_for_thread_num.f90 -o omp_parallel_for_thread_num_f_mp.exe
""" """

""" code w """
%%bash
OMP_NUM_THREADS=2 time ./omp_parallel_for_thread_num_f_mp.exe 10
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp omp_parallel_for_thread_num.cpp -o omp_parallel_for_thread_num_mp.exe
frtpx -Kfast -Kopenmp omp_parallel_for_thread_num.f90 -o omp_parallel_for_thread_num_f_mp.exe
```
"""

""" md
* 何度も書き換えてコンパイルするのが面倒になったら`schedule(runtime)` を使って, `OMP_SCHEDULE`をコマンドラインで指定するのが吉
* (上のソースの `schedule(static)` を `schedule(runtime)` に書き換えてコンパイルし直してから以下を実行する)
"""

""" code w """
%%bash
OMP_NUM_THREADS=2 OMP_SCHEDULE=static time ./omp_parallel_for_thread_num_mp.exe 10
""" """

""" code w """
%%bash
OMP_NUM_THREADS=2 OMP_SCHEDULE=dynamic time ./omp_parallel_for_thread_num_mp.exe 10
""" """

""" md
# `static` と `dynamic` の使い分け

* このプログラムでは, 繰り返し `i` の仕事量が `i` に比例して増えていく(`usleep(i * 100000)`)ので, 繰り返しごとに仕事量が大きく異なる
* `schedule(static)` だと, 大きい `i` を担当したスレッドに仕事が偏り, 他のスレッドが先に終わって待つことになる (<font color="blue">負荷の不均衡</font>)
* `schedule(dynamic)` だと, 早く終わったスレッドが次の繰り返しを取りに行くので, 負荷が均等になりやすい. 実行時間 (`time` の出力) を比べてみよ

* 一般的な目安
  * 各繰り返しの仕事量がほぼ等しいとわかっている場合は `static` が良い(割り当てを決めるオーバーヘッドがなく, データの局所性も良い)
  * 各繰り返しの仕事量がバラバラ, あるいは事前に予測できない場合は `dynamic` (や `guided`) が良い
  * ただし `dynamic` は割り当てのたびにスレッド間の調整が入るのでオーバーヘッドがある. 繰り返し1回が極端に短いときは `dynamic, c` でまとめて割り当てるとよい
"""
