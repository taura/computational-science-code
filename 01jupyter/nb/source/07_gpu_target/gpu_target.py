""" md

#* OpenMP GPU入門(1) --- `target` 構文 によるGPUオフロード

# OpenMP の GPU 向け拡張

* OpenMPの最近の仕様ではGPUもサポートしている(どこまでサポートしているかはコンパイラ依存)
* ここまでで `parallel` / `for` / `schedule` / `reduction` を使ってマルチコアCPUでの並列化を学んだ
* このトピックからはGPUを使った並列化を扱う
* C/C++ も Fortran も, 「並列化の指示を, 元の逐次プログラムにそっと付け加える」という発想は共通で, GPUでもそれは変わらない
  * C/C++ では `#pragma omp ...`
  * Fortran では `!$omp ...` ... `!$omp end ...`

* 詳しい仕様が知りたくなったら https://openmp.org/ を参照
  * 最新仕様 https://www.openmp.org/spec-html/5.2/openmp.html
  * 簡潔な文法のリファレンス: https://www.openmp.org/resources/refguides/

* このトピックで覚えるべきキーワード
  * `#pragma omp target` (Fortran: `!$omp target` ... `!$omp end target`)

* うまくするとCPUとGPUで同じソースコードで動くプログラムを書くことも可能

# 環境設定
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
# `#pragma omp target` $\sim$ 実行をデバイス(GPU)に移す

* <font color="blue">構文</font>
  * C/C++:
```
#pragma omp target
文
```
  * Fortran:
```
!$omp target
文の並び
!$omp end target
```
* 意味
  * 「文」(Fortranでは `!$omp target` から `!$omp end target` までの範囲)を<font color="blue">デバイス(通常はGPU)</font>で実行(にオフロード)する
  * GPUにオフロードするためには `-mp=gpu` をつけてコンパイルする

* 以下を実行すると, `target` 構文の中の `printf` (Fortranでは `print` 文) がGPU上で実行される
  * もっとも, `printf`/`print` の出力結果自体は, CPUで実行してもGPUで実行しても同じなので, 見た目では区別がつかないことに注意

## C++版
"""

""" code w """
%%writefile omp_target.cpp
""" include nb/source/07_gpu_target/include/omp_target.cpp """
""" """

""" md
* コンパイル
"""

""" code w """
%%bash
nvc++ -mp=gpu omp_target.cpp -o omp_target_gpu.exe
""" """

""" md
* ログインノードで実行
"""

""" code w """
%%bash
./omp_target_gpu.exe
""" """

""" md
## Fortran版
* 同じ内容をFortranで書くと以下のようになる
* C/C++の `#pragma omp target` ... (一つの文) に対し, Fortranでは `!$omp target` ... `!$omp end target` で範囲を囲む
"""

""" code w """
%%writefile omp_target.f90
""" include nb/source/07_gpu_target/include/omp_target.f90 """
""" """

""" md
* コンパイル
"""

""" code w """
%%bash
nvfortran -mp=gpu omp_target.f90 -o omp_target_f_gpu.exe
""" """

""" md
* ログインノードで実行
"""

""" code w """
%%bash
./omp_target_f_gpu.exe
""" """

""" md
* 以下ではいちいち示さないが参考のため Odyssey用のコンパイルオプション (C++ / Fortran)
  * ただし富士通コンパイラの `-Kopenmp` はマルチコアCPU向けであり, GPUオフロードはサポートしていない点に注意 (Odyssey環境にはGPUがない)
```
FCCpx -Kfast -Kopenmp omp_target.cpp -o omp_target_mp.exe
frtpx -Kfast -Kopenmp omp_target.f90 -o omp_target_f_mp.exe
```
"""

""" md
# `target` はGPUがなくても動く (CPUフォールバック)

* 注:
  * `target` は通常GPUを使用することを意図して使うが, 実際にはGPUがなくても実行できる(CPUにフォールバック実行)
  * 上記のプログラムを実行すると, マシンにGPUがあるかどうかに関係なく同じ結果が得られる
    * `printf`/`print` はどちらで実行しても同じなので当然
  * プログラムの移植性のためには良いが, GPUで実行しているつもりが実はCPU, などということがあるとかえって混乱を招く可能性がある
  * GPUが利用できなければエラーを発生させることもできる. それには, 環境変数 `OMP_TARGET_OFFLOAD=MANDATORY` を設定する. 逆に, `OMP_TARGET_OFFLOAD=DISABLED` はその反対の効果(必ずCPUで実行)を持つ

* GPUは計算ノードにのみ搭載されているので, GPUを使う実行は `%%bash_submit` でジョブとして投入する
* 必要に応じて rscgrp や elapse などを指定して実行せよ
* 例:
```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00
```

* 以下は `OMP_TARGET_OFFLOAD=MANDATORY` でGPUでの実行を強制する
  * ログインノードで (`%%bash` で) 実行するとGPUがないためエラーになる. 計算ノードに投入すること
"""

""" code w """
%%bash_submit
# GPUで実行することを強制. できなければエラー
OMP_TARGET_OFFLOAD=MANDATORY ./omp_target_gpu.exe
""" """

""" md
* 以下は計算ノードのCPU上で(GPUを使わずに)実行する
"""

""" code w """
%%bash_submit
# ホスト(CPU)で実行することを強制
OMP_TARGET_OFFLOAD=DISABLED ./omp_target_gpu.exe
""" """

""" md
* Fortran版でも同様に試せる
"""

""" code w """
%%bash_submit
# GPUで実行することを強制. できなければエラー
OMP_TARGET_OFFLOAD=MANDATORY ./omp_target_f_gpu.exe
""" """

""" md
# 終わりに一言

* これで「実行をGPUに移す」一歩を踏み出せた
* ただし `target` だけでは, GPU上で1スレッドが逐次に実行するだけで, GPUの性能はまったく引き出せていない
* GPUの多数のコア(Streaming Multiprocessor)とスレッドを使って並列実行するには, 次のトピックで扱う `teams` / `distribute` / `parallel` / `for` が必要になる
"""
