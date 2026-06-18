""" md 

#* 高性能プログラミングと性能測定(3) --- 練習問題 (マルチコア, GPU)

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
* 以下でコンパイラのパス名が fccpx is ..., nvc is ..., のように無事表示されれば成功
"""

""" code w """
%%bash
which nvc
which fccpx
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

# 1/8球の体積 を求めるプログラム

* 以下は3次元の1/8球 $B$ ($x^2 + y^2 + z^2 \leq 1$のうち, $x, y, z \geq 0$の部分)の体積を
$$ \int_B dx dy dz $$
によって求めるプログラムである
"""

""" code w """
%%writefile omp_ball_base.c
""" exec-include ./mk_version.py -D VER=1 nb/source/cs03/include/omp_ball.c """
""" """

""" code w """
%%bash
nvc -fast omp_ball_base.c -o omp_ball_base.exe
""" """

""" md 
* 第一引数(以下の512)は積分区間を何等分するかを決める
* 大きくすると精度と計算量が上がる(第一引数$=n$として$n^3$に比例)
* 答えは当然ながら
$$ \frac{1}{8} \times \frac{4}{3}\pi = \frac{\pi}{6} $$
の近似値になる
* 以下の512を変えて精度(error)と実行時間が変わることを確認せよ
"""

""" code w """
%%bash_submit
./omp_ball_base.exe 512
""" """

""" md 

#*P マルチコア並列化

* 上記をOpenMPを使ってマルチコアCPU用に並列化せよ

"""

""" code w """
%%writefile omp_ball_mp.c
""" exec-include ./mk_version.py -D VER=1 nb/source/cs03/include/omp_ball.c """
""" """

""" code w """
%%bash
nvc -fast -mp=multicore omp_ball_mp.c -o omp_ball_mp.exe
""" """

""" md 
* 以下の`OMP_NUM_THREADS=1` を色々変えて実行してみよ
"""

""" code w """
%%bash_submit
OMP_NUM_THREADS=1 ./omp_ball_mp.exe 512
""" """

""" md 
* 以下を適切に修正して様々なスレッド数で実行し, 
"""

""" code w """
%%bash_submit
#PJM -L gpu=4
#PJM --omp thread=36
#PJM -L elapse=0:01:00
for th in 1 2 適切にスレッド数を並べよ ; do
  echo "====="
  OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./omp_ball_mp.exe 512
done
""" """

""" md 
* 結果を以下にコピペして性能向上を可視化せよ(性能向上がすぐに頭打ちになるようであれば, $n$の値を調節せよ)
"""

""" code w """
""" include nb/source/cs03/include/speedup.py """
""" """

""" md 

#*P GPU並列化

* 同じプログラムをOpenMPを使ってGPU用に並列化せよ

"""

""" code w """
%%writefile omp_ball_gpu.c
""" exec-include ./mk_version.py -D VER=1 nb/source/cs03/include/omp_ball.c """
""" """

""" code w """
%%bash
nvc -fast -mp=gpu omp_ball_gpu.c -o omp_ball_gpu.exe
""" """

""" md 
* 以下の`OMP_NUM_THREADS=1`, `OMP_NUM_TEMAS=1` を色々変えてやってみよ
* `./omp_ball_gpu.exe` で `OMP_NUM_THREADS=1` としたときと, `./omp_ball_gpu.exe` で`OMP_NUM_TEAMS=1 OMP_NUM_THREADS=1` としたときの性能の違いに注目
* はじめに`OMP_NUM_TEMAS=1` に固定したまま `OMP_NUM_THREADS=1` を増やし, 性能が頭打ちにになる `OMP_NUM_THREADS`の値を求めよ
* 第一引数($n$)の値は適宜変えて, 実行時間があまりにも短くならないようにせよ
"""

""" code w """
%%bash_submit
OMP_TARGET_OFFLOAD=MANDATORY OMP_NUM_TEAMS=1 OMP_NUM_THREADS=1 ./omp_ball_gpu.exe 512
""" """

""" md 
* 以下を適切に修正して様々なスレッド数で実行し, 
"""

""" code w """
%%bash_submit
for th in 1 32 64 適切にスレッド数を並べよ ; do
  echo "====="
  OMP_TARGET_OFFLOAD=MANDATORY OMP_NUM_TEAMS=1 OMP_NUM_THREADS=${th} ./omp_ball_gpu.exe 512
done
""" """

""" md 
* 結果を以下にコピペして性能向上を可視化せよ(性能向上がすぐに頭打ちになるようであれば, $n$の値を調節せよ)
"""

""" code w """
DATA = r'''
ここにデータを貼り付け(この行は消す)
'''

speedup(DATA)
""" """

""" md 
* 次に`OMP_NUM_THREADS` をその値に固定したまま `OMP_NUM_TEMAS` を増やして性能の向上を観察せよ
* 以下の$n$も必要ならば適切な値に修正せよ
"""

""" code w """
%%bash_submit
OMP_TARGET_OFFLOAD=MANDATORY OMP_NUM_TEAMS=1 OMP_NUM_THREADS=適切なスレッド数 ./omp_ball_gpu.exe 512
""" """

""" md 
* 以下を適切に修正して様々なチーム数で実行し, 
"""

""" code w """
%%bash_submit
th=適切なスレッド数
for tm in 1 2 3 適切にチーム数を並べよ ; do
  echo "====="
  OMP_TARGET_OFFLOAD=MANDATORY OMP_NUM_TEAMS=${tm} OMP_NUM_THREADS=${th} ./omp_ball_gpu.exe 512
done
""" """

""" md 
* 結果を以下にコピペして性能向上を可視化せよ(性能向上がすぐに頭打ちになるようであれば, $n$の値を調節せよ)
"""

""" code w """
DATA = r'''
ここにデータを貼り付け(この行は消す)
'''

speedup(DATA)
""" """

""" md 
# 行った計算方法に対するいくつかの注

* 今回, 
$$ \int_B dx dy dz $$
を計算するのに, $B$が3次元であるのに合わせて3重ループを用いているがもちろん, 球の体積を求めることだけが目的ならばその必要はない

* 以下のような2重積分を行えば良く, その方が当然ながら計算量が少ない
$$ \int_{B_{xy}} \sqrt{1 - x^2 - y^2} dx dy dz $$

* 一方ここで行った方法は, $B$が複雑な式($x, y, z$のどれについても容易に解けない式)であっても一般に使えるという利点がある. 具体的には
$$ f(x, y, z) \leq 0 $$
の体積を求めるのに, 上記を満たす$x, y, z$のbounding boxがわかりさえすればよい

* そのような場合に,
  1. bounding box内に, 一様に多数の点を生成
  1. それらの点が, $f(x, y, z) \leq 0$ を満たすか否かを調べ, その割合$r$を求める
  1. $f(x, y, z) \leq 0$ の体積 $\approx$ bounding boxの体積 $\times r$

* なお今回はその「多数の点」として, $[0, 1]^3$の3辺をすべて$n$等分し, できた$n^3$個の小立方体の中心を用いた($((i+0.5)/n, (j+0.5)/n, (k+0.5)/n)$)が, 実はそうする必要はなく, 乱数を用いる方法が一般的(**モンテカルロ法による積分**)

"""

