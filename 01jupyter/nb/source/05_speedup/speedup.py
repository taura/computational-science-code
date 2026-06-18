""" md

#* OpenMP入門(4) --- 台数効果の測定

# おさらい

* ここまでで, `parallel` / `for` / `schedule` を使ってループを複数スレッドで分割実行できるようになった
* このトピックでは, スレッド数を増やすと実際にどれだけ速くなるか(<font color="blue">台数効果</font>, スピードアップ)を測定する
* 性能の指標として GFLOPS (1秒あたり何十億回の浮動小数点演算ができたか) を用いる

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
# 台数効果とは

* $T$個のスレッドを用いても速度が$T$倍(近く)にならないことはしばしば(「ほとんど」というべきか)
* そもそも$T > $コア数のときは複数のスレッドが同じコア上で動くことになり, コア数以降の向上は望めない(スレッドがわざと(usleepで)休眠していたり, ファイルのIOなどで長時間止まっている場合は別)
* したがってOpenMPをそのような設定で使うことはあまりない
* $T \leq $コア数であっても, 速度が$T$倍 (実行時間が$1/T$倍) とは程遠くなる理由がいくつも有る
  1. 1つのスレッドによって実行される処理が小さすぎる ($T$個スレッドが実行を開始して, 全員が終了するのを待つ, というオーバーヘッドが目立つ)
  1. OSが速やかに$T$個のスレッドを別々のコアに割り当ててくれない
  1. スレッド間でデータを共有している場合にデータアクセスのコストが1スレッドの場合に比べて大きくなる(詳細は計算機の仕組みに関わるのでここでは深入りしない)
* 3番目はアルゴリズムの本質に根ざした問題で容易に除去できない場合が多いが,
  * 1についてはそういうものだと思っておく(あまり短すぎる処理を複数コアで性能向上はできない. あまりそうする意味もないのでやる必要もない)
  * 2については, 実行時に`OMP_PROC_BIND=true` と環境変数を設定することで改善することがある. `OMP_PROC_BIND=true` は, 各スレッドを特定のコアでしか実行しない, かつそれらが均等になるという効果を持つ指示で, 2の問題を緩和できる. 説明: https://www.openmp.org/spec-html/5.0/openmpse52.html

* 以下では, 計算自身にあまり意味はないが簡単な例題で性能向上を目撃してみる
* `lin_rec` は `x = a * x + b` を$n$回繰り返す関数で, 1回につき乗算1回・加算1回, 計$2n$回の浮動小数点演算を行う
* それを$m$個の独立な要素について並列に計算する ( `#pragma omp parallel for` / `!$omp parallel do` )

## C++版
"""

""" code w """
%%writefile omp_speedup.cpp
""" include nb/source/05_speedup/include/omp_speedup.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=multicore omp_speedup.cpp -o omp_speedup_mp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_speedup.f90
""" include nb/source/05_speedup/include/omp_speedup.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore omp_speedup.f90 -o omp_speedup_f_mp.exe
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
```
FCCpx -Kfast -Kopenmp omp_speedup.cpp -o omp_speedup_mp.exe
frtpx -Kfast -Kopenmp omp_speedup.f90 -o omp_speedup_f_mp.exe
```
"""

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
* 直線が「理想的な台数効果(スレッド数に比例)」, もう一方が実測値
"""

""" code w """
""" include nb/source/05_speedup/include/speedup.py """
""" """

""" md
* なおこの実験はいろいろな意味でいい加減な実験だということを注意しておく
  * 本来は同じ条件で何度も実験して, ばらつきなどを見つつ平均をとるべき
  * `OMP_PROC_BIND=true` を設定して複数のスレッドが同一コアに割り当たらないようにしているが, 同時に複数の人が実験すると, 同一コアに複数の人のスレッドが割り当てられることがありうるので実行したタイミングの運・不運で結果が変わる
  * 本来は一回のプログラム中でも同じループを複数回実行して計測するのが正しい
  * 特に何事も「プログラム中の最初の一回の◯◯」というのは特別な処理が必要になりがちで, 実際OpenMPでも初めて`parallel`構文に遭遇したときにOSのスレッドが生成され, それ以降は同じスレッドが使い回されるということが通常である(つまり初めての `parallel` はそれ以降より余分なオーバーヘッドがかかりやすい)
  * など
"""
