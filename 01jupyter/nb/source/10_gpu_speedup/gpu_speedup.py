""" md

#* OpenMP GPU入門(4) --- GPU上での台数効果の測定

# おさらい

* ここまでで, `target` / `teams` / `distribute` / `parallel` / `for` (`do`) を使って, ループをGPU上の多数のスレッドで分割実行できるようになった
* このトピックでは, CPUのときと同様, スレッド数(さらにチーム数)を増やすとGPU上で実際にどれだけ速くなるか(<font color="blue">台数効果</font>, スピードアップ)を測定する
* 性能の指標として GFLOPS (1秒あたり何十億回の浮動小数点演算ができたか) を用いる

# 環境設定

* これまでのトピックと同様, コンパイラとジョブ投入の設定を行う
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
# 台数効果とは (GPU編)

* GPUでは, $T$個のチーム × $H$個のスレッド = $T \times H$ 個のスレッドが並列に動く
* CPUのときと同じように, 並列度(スレッド数・チーム数)を増やしても, それに比例して速くなるとは限らない
* GPUの場合, 1個のチーム(Streaming Multiprocessor, SM上で実行される)が抱えるスレッド数や, 全体のチーム数が少なすぎると, GPUの演算器を十分に使い切れず, 性能が出ない
* 逆にハードウェアの並列度を超えてチーム数・スレッド数を増やしても, それ以上は速くならない
* ここでは, 計算自身にあまり意味はないが簡単な例題で, GPU上での性能向上を目撃してみる
* `lin_rec` は `x = a * x + b` を$n$回繰り返す関数で, 1回につき乗算1回・加算1回, 計$2n$回の浮動小数点演算を行う
* それを$m$個の独立な要素について並列に計算する ( `#pragma omp target teams distribute parallel for` / `!$omp target teams distribute parallel do` )

```
OMP_NUM_TEAMS=nteams OMP_NUM_THREADS=nthreads ./omp_speedup_gpu.exe m n
```
* とすると, チーム数=`nteams`, スレッド数=`nthreads` で実行する
* `m`, `n` を省略すると, `m` = `nteams` $\times$ `nthreads`, `n` = $100 \times 1000 \times 1000$ とする

## C++版
"""

""" code w """
%%writefile omp_speedup.cpp
""" include nb/source/10_gpu_speedup/include/omp_speedup.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -mp=gpu omp_speedup.cpp -o omp_speedup_gpu.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_speedup.f90
""" include nb/source/10_gpu_speedup/include/omp_speedup.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=gpu omp_speedup.f90 -o omp_speedup_f_gpu.exe
""" """

""" md
# スレッド数を変えてみる

* GPUは計算ノードにのみ搭載されているので, GPUを使う実行は `%%bash_submit` でジョブとして投入する
* 必要に応じて rscgrp や elapse などを指定して実行せよ
* まず, `OMP_NUM_TEAMS=1` (チーム数1) に固定し, `OMP_NUM_THREADS` (スレッド数) だけを変えて性能向上を確認する
* <font color="red">`OMP_NUM_THREADS` は 1 でなければ, 32 の倍数でないといけないことに注意</font> (GPUのスレッドは32本単位(ワープ)で動くため)
* まずは1チーム・1スレッドで実行してみる
"""

""" code w """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=1 OMP_NUM_THREADS=1 ./omp_speedup_gpu.exe
""" """

""" md
* 手動でやるのが嫌になったら以下で一撃で実行 (スレッド数を 1, 32, 64, ... と増やしていく)
"""

""" code w """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

for th in 1 32 64 128 256 512 1024 ; do
    echo -n "$th "
    OMP_NUM_TEAMS=1 OMP_NUM_THREADS=${th} ./omp_speedup_gpu.exe | grep GFLOPS
done
""" """

""" md
* 結果を以下で可視化 (上の結果をコピペせよ)
* 直線が「理想的な台数効果(スレッド数に比例)」, もう一方が実測値
"""

""" code w """
""" include nb/source/10_gpu_speedup/include/speedup.py """
""" """

""" md
# チーム数を変えてみる

* 次に, スレッド数を上記で性能が頭打ちになった(あるいは最も良かった)値で固定した上で, `OMP_NUM_TEAMS` (チーム数) を色々変えて実行する
* まず固定した1チームでの基準を測る
"""

""" code w """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=1 OMP_NUM_THREADS=最適なスレッド数 ./omp_speedup_gpu.exe
""" """

""" md
* 手動でやるのが嫌になったら以下で一撃で実行 (チーム数 `tm` を増やしていく)
* このGPU (NVIDIA A100) は 108 個の Streaming Multiprocessor (= チームを割り当てられる単位) を備えているので, おおむね 108 までチーム数を増やすと性能が向上し, それ以降は頭打ちになるはず
"""

""" code w """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

th=最適なスレッド数
for tm in 1 2 4 8 16 32 64 108 ; do
    echo -n "$((tm * th)) "
    OMP_NUM_TEAMS=${tm} OMP_NUM_THREADS=${th} ./omp_speedup_gpu.exe | grep GFLOPS
done
""" """

""" md
* 結果を以下で可視化 (上の結果をコピペせよ)
* 横軸は (チーム数 × スレッド数), すなわち総スレッド数になっている
"""

""" code w """
""" include nb/source/10_gpu_speedup/include/speedup.py """
""" """

""" md
# まとめ

* GPUでは, 1チームあたりのスレッド数(`OMP_NUM_THREADS`)と, チーム数(`OMP_NUM_TEAMS`)の両方を十分に大きくして, 初めてGPUの性能を引き出せる
* `OMP_NUM_THREADS` は 1 または 32 の倍数でなければならない
* チーム数は GPU の Streaming Multiprocessor 数 (A100 では 108) 程度までは性能向上が期待できる
* なお, CPUのときと同様に, この実験はいろいろな意味でいい加減な計測であることに注意 (本来は複数回実行して平均をとる, 最初の1回はオフロードの初期化コストが余分にかかる, など)
"""
