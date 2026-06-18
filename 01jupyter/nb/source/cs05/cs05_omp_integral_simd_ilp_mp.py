""" md

#* 高性能プログラミングと性能測定(5) --- 練習問題 (SIMD + ILP + マルチコア)

"""

""" md
# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと

## コンパイラ

* Aquariusでは, 同じコンパイラでCPUもGPUもサポートしているという理由で, NVIDIA HPC SDKを使う
  * コマンド名:
    * C: `nvc`
    * C++: `nvc++`
  * コンパイルオプション:
    * `-mp=multicore` をつけると CPU用のOpenMPがサポートされる
    * `-mp=gpu` をつけると GPU用のOpenMPがサポートされる
* Odysseyでは, 富士通コンパイラを使う
  * コマンド名:
    * C: `fccpx`
    * C++: `FCCpx`
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

# 1/8球の体積 を求めるプログラム (SIMD + ILP + マルチコア)

"""

""" md 

#*P SIMD, ILP, マルチコア並列化

* cs03で行った1/8球の体積を求めるプログラムを, SIMD, ILP, マルチコアすべてを利用して高速化せよ

"""

""" code w """
%%writefile omp_ball_simd_ilp_mp.c
""" exec-include ./mk_version.py -D VER=1 nb/source/cs03/include/omp_ball.c """
""" """

""" md 

* ヒント
  * このプログラムのベクトル化で一つ厄介なのは, `x * x + y * y + z * z < 1.0` の部分
  * `A < B` は, 満たされていれば1, なければ0という式
  * ここで`A, B` がベクトル型だったら `A < B` が 「`A[i] < B[i]`を満たす要素の数」だったりしたら話が早いのだがそうは問屋がおろさず, そもそもそのような演算子 (ベクトル型同士の大小比較) というものが許されていない
  * したがって, `count_lt(double A, double B)` とでも名付けて, 上記の値を返す関数を自分で作る必要がある
"""

""" code w """
%%bash
nvc -fast -mp=multicore -Minfo -Mneginfo omp_ball_simd_ilp_mp.c -o omp_ball_simd_ilp_mp.exe
""" """

""" md 
* 以下の`OMP_NUM_THREADS=1` を色々変えて実行してみよ
"""

""" code w """
%%bash_submit
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./omp_ball_simd_ilp_mp.exe 512
""" """

""" md 
* 以下を適切に修正して様々なスレッド数で実行し, 
"""

""" code w """
%%bash_submit
#PJM -L gpu=4
#PJM --omp thread=36
#PJM -L elapse=0:05:00
for th in 1 2 適切にスレッド数を並べよ ; do
  echo "====="
  OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./omp_ball_simd_ilp_mp.exe 512
done
""" """

""" md 
* 結果を以下にコピペして性能向上を可視化せよ(性能向上がすぐに頭打ちになるようであれば, $n$の値を調節せよ)
* GPUでの結果と比較せよ
"""

""" code w """
""" include nb/source/cs03/include/speedup.py """
""" """

