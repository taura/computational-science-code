""" md

#* CPU SIMD入門(3) --- 命令レベル並列性(ILP)とSIMD・マルチコアの組み合わせ

# おさらい

* ここまでで, マルチコア(`parallel for`), GPU(`target`), そして CPU の SIMD(自動ベクトル化・`omp simd`・ベクトル型拡張)を学んできた
* このトピックでは,
  * まず SIMD によって実際にどれだけ性能が向上するかを<font color="blue">目撃</font>する
  * 次に, ベクトル長 `nl` を 2 倍, 4 倍, 8 倍... と<font color="blue">大きくする</font>と, <font color="blue">命令レベル並列性(ILP, instruction-level parallelism)</font>が引き出され, 1 コアの限界性能まで性能が伸びる場合があることを見る
  * 最後に, SIMD + ILP + マルチコアの<font color="blue">すべてを組み合わせ</font>て性能を最大化し, GPU の最大性能と比べる
* `nl` を大きくすることで 1 コアの性能が伸びるのは, GPU が 1 つのコアの中で多数のスレッドを走らせて性能を出すのと<font color="blue">本質的に同じ理由</font>である(後で説明)

* このトピックで覚えるべきキーワード
  * 命令レベル並列性 (ILP)
  * ベクトル長 `nl` の調整による性能向上

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
# SIMDによる性能向上の目撃

* マルチコアやGPUでも用いた以下のプログラムを, SIMD を用いて高速化する
* `lin_rec` は `t = a * t + b` を $n$ 回繰り返す関数で, 1 回につき乗算 1 回・加算 1 回, 計 $2n$ 回の浮動小数点演算を行う
* それを $m$ 個の独立な要素について計算する

* まずは SIMD を使わない<font color="blue">スカラ版(ベースライン)</font>
"""

""" code w """
%%writefile omp_speedup_base.cpp
""" include nb/source/13_ilp/include/omp_speedup_base.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_base.exe omp_speedup_base.cpp
""" """

""" code w """
%%bash
./omp_speedup_base.exe 64 $((100 * 1000 * 1000))
""" """

""" md
* 目標は以下の外側のforループ
```
for (long i = 0; i < m; i++) {
  x[i] = lin_rec(0.99, i + 1, 1.0, n);
}
```
の, 連続する `nl` 回(`double` なので 512 bit / 64 bit = 8 回)の繰り返しをまとめて実行することである
* 擬似的に書けば(`nl = 8`, `m` は `nl` で割り切れると仮定して)
```
for (long i = 0; i < m; i += nl) {       // nl 回ずつまとめて
  x[i:i+nl] = lin_rec(0.99, (i:i+nl) + 1, 1.0, n);
}
```
* 前のトピックで定義した `range` / `storev` を借りて書けば
```
for (long i = 0; i < m; i += nl) {
  storev(&x[i], lin_rec(0.99, range(i) + 1, 1.0, n));
}
```
* `lin_rec` も, 引数 `b`(繰り返しごとに異なる)と作業変数 `t` をベクトル型 `doublev` にする
```
doublev lin_rec(double a, doublev b, double c, long n) {
  doublev t = uniform(c);
  for (long j = 0; j < n; j++) {
    t = a * t + b;
  }
  return t;
}
```
* つまり `nl` 個の独立な漸化式を, ベクトル型のまま<font color="blue">同時に</font>進めることになる
* 全体をまとめると以下の通り
"""

""" code w """
%%writefile omp_speedup_simd.cpp
""" include nb/source/13_ilp/include/omp_speedup_simd.cpp """
""" """

""" code w """
%%bash
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd.exe omp_speedup_simd.cpp
""" """

""" code w """
%%bash
./omp_speedup_simd.exe 64 $((100 * 1000 * 1000))
""" """

""" md
* スカラ版に比べて GFLOPS 値が向上していることを確認せよ

# 命令レベル並列性(ILP)の向上

* 上記のコードには<font color="blue">さらなる性能向上の余地</font>がある
* `omp_speedup_simd.cpp` の冒頭の
```
enum { nl = 8 };
```
の `nl` を 2 倍, 4 倍, 8 倍... (8, 16, 32, 64, ...)と大きくして<font color="blue">再コンパイル</font>すると, さらに性能が向上する場合がある
* 実際に `nl` の値を変えて, 最大性能が得られる値を探してみよ(`m` も `nl` で割り切れるように, 例えば `nl` の倍数にしておくとよい)
* なお, あまり本質的とは思えないが, NVIDIAコンパイラでは `nl` として 2 のべき乗(1, 2, 4, 8, 16, ...)以外の値は許さないという制限がある(このせいで実際に最適な値に設定できない場合もある)
"""

""" code w """
%%bash
# 例: nl を編集して再コンパイル・再実行することを繰り返す
nvc++ -fast -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd.exe omp_speedup_simd.cpp
./omp_speedup_simd.exe 64 $((100 * 1000 * 1000))
""" """

""" md
* なぜ `nl` を大きくすると(SIMD レジスタの幅 8 を超えても)性能が上がるのか:
  * `lin_rec` の内側ループ `t = a * t + b` は, 次の繰り返しが<font color="red">前の `t` の結果に依存</font>している(漸化式)
  * 1 つの変数 `t` だけをひたすら更新すると, 1 回の `t = a * t + b` の<font color="red">演算の遅延(latency)</font>が終わるまで次の演算を始められず, 演算器(FPU)に十分なデータが供給されない --- 演算器が遊んでしまう
  * `nl` を大きくする = <font color="blue">互いに独立な漸化式(レーン)を多数</font>同時に進める, ということ
  * 独立なレーンが多ければ, あるレーンの演算の遅延を待つ間に別のレーンの演算を投入でき, 演算器を埋め尽くせる --- これが<font color="blue">命令レベル並列性(ILP)</font>である
  * `nl` が SIMD 幅(8)の何倍かのとき, 1 つのベクトル型は複数の SIMD レジスタに分かれて格納され, それらが独立に演算されることで遅延が隠蔽される
* これは, <font color="blue">GPU が 1 つのコアの中で多数のスレッドを切り替えながら走らせて, メモリや演算の遅延を隠して性能を出すのと本質的に同じ理由</font>である
* ただし `nl` を大きくしすぎると, レジスタが足りなくなる(レジスタスピル)などで逆に遅くなるので, 適度なところに最大値がある

# SIMD + ILP + マルチコアの組み合わせ

* ここまでは 1 コアでの SIMD と ILP だった
* さらに外側の(独立な要素についての)ループを `#pragma omp parallel for` で<font color="blue">マルチコア並列化</font>すれば, SIMD + ILP + マルチコアのすべてを組み合わせられる
"""

""" code w """
%%writefile omp_speedup_simd_mp.cpp
""" include nb/source/13_ilp/include/omp_speedup_simd_mp.cpp """
""" """

""" md
* マルチコア用に `-mp=multicore` をつけてコンパイルする
"""

""" code w """
%%bash
nvc++ -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd_mp.exe omp_speedup_simd_mp.cpp
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション
```
FCCpx -Kfast -Kopenmp omp_speedup_simd_mp.cpp -o omp_speedup_simd_mp.exe
```
"""

""" md
* まずは 1 スレッドで実行
* `OMP_PROC_BIND=true` は各スレッドを特定のコアに固定し, 台数効果の測定を安定させる指示である
"""

""" code w """
%%bash
OMP_PROC_BIND=true OMP_NUM_THREADS=1 ./omp_speedup_simd_mp.exe 64 $((100 * 1000 * 1000))
""" """

""" md
* スレッド数を変えて性能向上を測定する
* スレッド数 `th` に応じて `m`(第1引数)を `64 * th` のように増やし, 1 スレッドあたりの仕事量を一定に保っている
* (性能向上がすぐに頭打ちになるようであれば `m` や `n` の値を調節せよ)
"""

""" code w """
%%bash
for th in 1 2 3 4 6 8 9 ; do
    echo -n "$th "
    OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./omp_speedup_simd_mp.exe $((64 * ${th})) $((100 * 1000 * 1000)) | grep GFLOPS
done
""" """

""" md
* Wisteria Aquarius の計算ノードは 72 コアで, 8 GPUを搭載しており, 1 GPUにつき 72 / 8 = 9 コアが割り当てられる
* それ以上のコア(最大 36 コアなど)を使いたい場合は, 以下のようにジョブとして投入する
"""

""" code w """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L gpu=4
#PJM --omp thread=36
#PJM -L elapse=0:05:00

for th in 1 2 3 4 6 8 9 12 18 24 36 ; do
    echo -n "$th "
    OMP_PROC_BIND=true OMP_NUM_THREADS=${th} ./omp_speedup_simd_mp.exe $((64 * ${th})) $((100 * 1000 * 1000)) | grep GFLOPS
done
""" """

""" md
* 結果を以下にコピペして性能向上を可視化せよ
* 直線が「理想的な台数効果(スレッド数に比例)」, もう一方が実測値
"""

""" code w """
""" include nb/source/13_ilp/include/speedup.py """
""" """

""" md
* 得られた最大性能(SIMD + ILP + マルチコアの組み合わせ)を, GPU トピックで得られた GPU 上の最大性能と比べてみよ

# Fortranでは

* <font color="red">重要:</font> ここで用いた `__attribute__((vector_size(N)))` によるベクトル型は<font color="red">C/C++独自の拡張</font>であり, Fortranには相当する機能が無い
* したがって Fortran では, ベクトル型で明示的に ILP を制御する代わりに,
  * 内側の漸化式ループを `!$omp simd` で<font color="blue">SIMD 化</font>し,
  * 外側の(独立な要素についての)ループを `!$omp parallel do` で<font color="blue">マルチコア並列化</font>して,
  * SIMD・ILP の引き出しはコンパイラに委ねる
* 「望み通りの幅の SIMD 命令が出るか」「どこまで ILP が引き出せるか」は C/C++ のベクトル型ほど制御できないので, `-Minfo` の出力で確認することが大切である
* 以下に Fortran 版の例を示す
"""

""" code w """
%%writefile omp_speedup_simd_mp.f90
""" include nb/source/13_ilp/include/omp_speedup_simd_mp.f90 """
""" """

""" code w """
%%bash
nvfortran -fast -mp=multicore -Mkeepasm -Minfo -Mneginfo -o omp_speedup_simd_mp_f.exe omp_speedup_simd_mp.f90
""" """

""" code w """
%%bash
OMP_PROC_BIND=true OMP_NUM_THREADS=4 ./omp_speedup_simd_mp_f.exe 64 $((100 * 1000 * 1000))
""" """

""" md
* `-Minfo` の出力に "Generated vector ..." のようなメッセージが出ていれば内側ループの SIMD 化に成功している
"""
