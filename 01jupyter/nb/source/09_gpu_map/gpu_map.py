""" md

#* OpenMP GPU入門(3) --- map 句によるデータ移動

# なぜデータ移動が必要か

* GPUはCPUと異なるメモリを持っていて, (CPU内のコア同士と異なり)メモリをハードウェア的に共有しているわけではない(実は最近この事情は変わりつつあるが, 少なくともこの環境やWisteriaはそうではない)
* したがってある計算をGPU上で行おうと思ったら, 一般にはその計算が必要とするデータをGPUに送ってから実行する必要がある
* 逆にGPU上で得た結果をCPUで使いたければ, GPUが計算した結果をCPUに送る必要がある
* `target data` 構文とその `map` 節は, それらのデータ転送を手軽に行う指示である

* このトピックで覚えるべきキーワード
  * `map(to: ...)`, `map(from: ...)`, `map(tofrom: ...)`
  * `#pragma omp target data` (Fortran: `!$omp target data` ... `!$omp end target data`)
  * これらの `map` 節は `#pragma omp target` (Fortran: `!$omp target`) に直接付けることもできる

# 環境設定

* Jupyter上でコンパイラを起動する, およびジョブ投入を簡便にするための設定
* これは各Jupyterノートブックごとに行う
* 同じノートブックでもログアウトしたりカーネルを再スタートしたときなどは失われるのでそのたびに行うこと
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
# `map` 節と `target data` 構文

* <font color="red">警告：</font> [仕様書](https://www.openmp.org/spec-html/5.0/openmpsu109.html#x142-6180002.19.7)は法律のようなわかりにくさ
* 以下はより直截で, 普通の日本語で説明を試みているが, 一部は仕様書で裏を取らず, 実際の実験や実装が行っているであろうことの想像に基づいて書かれている

* <font color="blue">構文：</font>
  * C/C++:
```
#pragma omp target data map(to: ...) map(from: ...) map(tofrom: ...) ...
    S
```
  * Fortran:
```
!$omp target data map(to: ...) map(from: ...) map(tofrom: ...) ...
    文の並び S
!$omp end target data
```
ここで, ... は変数, 配列名, または基底アドレス + 範囲 (例: C/C++では `a[0:n]`, Fortranでは `a` や `a(1:n)`) など

* これらの節は指定された変数, 配列, またはアドレス範囲が $S$ の間または後に「期待される」値を持つという効果をもたらす
* より具体的には,
  * `map(to: ...)` に指定されたものは $S$ の間GPU上で有効になる
  * `map(from: ...)` に指定されたものは $S$ の後CPU上で有効になる
* これを達成するために, 実行時システムによってCPUアドレスとGPUアドレス間の <font color="blue">_マッピング_</font> が維持され, 必要に応じて内容がCPU-GPU間で移動する(コピーされる)
  * `map(to: ...)` に指定されたデータは $S$ の前に必要ならばGPUにコピーされる (CPU -> GPU)
  * `map(from: ...)` に指定されたデータは $S$ の後に必要ならばGPUからコピーされる (GPU -> CPU)
  * `map(tofrom: ...)` は両方の効果を持つ
* `map` 節を指定しなくても同じ効果を持つこともある(が, どういうときにそうなるかを仕様書から理解しようとするよりも, 指定するほうが早い)

* 通常, このディレクティブは `#pragma omp target` (`!$omp target`) と一緒に使用され, 実際にこれらの節を `#pragma omp target` (`!$omp target`) に直接指定することもできる

## 配列の範囲指定について

* C/C++では配列の一部をマップするとき `a[0:n]` のように「先頭の添字:要素数」で書く (要素数であって最後の添字ではないことに注意)
* Fortranでは配列全体は単に `a`, 一部は `a(1:n)` のように書く

# `map` 節を付けない場合 --- GPUの書き換えはCPUに戻らない

* 以下のプログラムは, スカラ `t`, 静的配列 `a`, 構造体(派生型) `p` を用意し,
  * まずGPU上でそれらの値を表示し,
  * GPU上でそれらを2倍に書き換え,
  * その後CPU上でそれらの値を表示する
* `t`, `a`, `p` はスカラや静的配列なので, `map` 節を書かなくてもGPU上で利用可能になる(暗黙にマップされる)
* しかし `map` 節を指定していないので, GPUで書き換えた結果はCPUには戻ってこない
* 実行して, CPU側の表示が(2倍にならず)元のままであることを観察せよ

## C++版
"""

""" code w """
%%writefile omp_map_local.cpp
""" include nb/source/09_gpu_map/include/omp_map_local.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_map_local.cpp -o omp_map_local_gpu.exe
""" """

""" code w """
%%bash_submit
./omp_map_local_gpu.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_map_local.f90
""" include nb/source/09_gpu_map/include/omp_map_local.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_map_local.f90 -o omp_map_local_f_gpu.exe
""" """

""" code w """
%%bash_submit
./omp_map_local_f_gpu.exe
""" """

""" md
# `map(tofrom: ...)` を付けた場合 --- GPUの書き換えがCPUに戻る

* 先ほどのプログラムに `map(tofrom: t, a, p)` を加えたものが以下である
  * C/C++では配列の範囲を `a[0:3]` のように指定する
  * Fortranでは配列全体を単に `a` と書ける
* これにより, GPUが書き換えた結果が `target` 領域の後にCPUに戻ってくる
* 実行して, 今度はCPU側の表示が2倍になっていることを観察せよ
* なお, 値を戻すだけなら `map(tofrom: ...)` の代わりに `map(from: ...)` でもよい(GPUへ送る必要がなく, 結果だけ欲しい場合)

## C++版
"""

""" code w """
%%writefile omp_map_tofrom.cpp
""" include nb/source/09_gpu_map/include/omp_map_tofrom.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_map_tofrom.cpp -o omp_map_tofrom_gpu.exe
""" """

""" code w """
%%bash_submit
./omp_map_tofrom_gpu.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_map_tofrom.f90
""" include nb/source/09_gpu_map/include/omp_map_tofrom.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_map_tofrom.f90 -o omp_map_tofrom_f_gpu.exe
""" """

""" code w """
%%bash_submit
./omp_map_tofrom_f_gpu.exe
""" """
