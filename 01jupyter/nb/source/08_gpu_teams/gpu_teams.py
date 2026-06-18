""" md

#* OpenMP GPU入門(2) --- teams, distribute, parallel, for

# おさらいと今回のテーマ

* 前のトピックで `#pragma omp target` (Fortran: `!$omp target`) によって実行をGPUに移せること, GPU上では多数の<font color="blue">チーム</font>が作られることを見た
* このトピックでは, GPU上での並列化の「階層」を担う4つのキーワードを整理する
  * `teams` ($\approx$ `parallel`) --- 多数の<font color="blue">チーム</font>を作る
  * `distribute` ($\approx$ `for`) --- ループの繰り返しを<font color="blue">チーム間</font>で分割する
  * `parallel` --- 各チームの中に<font color="blue">スレッド</font>を作る
  * `for` (Fortran: `do`) --- ループの繰り返しを<font color="blue">チーム内のスレッド間</font>で分割する

* ディレクティブの対応関係のイメージ

| CPU (マルチコア) | GPU |
|---|---|
| `parallel` (スレッドを作る) | `teams` (チームを作る) + `parallel` (各チーム内でスレッドを作る) |
| `for` (繰り返しをスレッドに分割) | `distribute` (繰り返しをチームに分割) + `for` (繰り返しをスレッドに分割) |

* つまりGPUでは「チーム」と「スレッド」という2階層の並列性があり,
  * 外側の階層 = `teams` / `distribute` でチームに仕事を配り,
  * 内側の階層 = `parallel` / `for` で各チームの中のスレッドに仕事を配る
* と考えればよい

* このトピックで覚えるべきキーワード (C/C++ では `#pragma omp ...`, Fortran では `!$omp ...`)
  * `teams`, `distribute`, `parallel`, `for` (Fortran: `do`)
* このトピックで覚えるべきAPI関数 (C/C++ では `#include <omp.h>`, Fortran では `use omp_lib`)
  * `omp_get_num_teams();` / `omp_get_team_num();`
  * `omp_get_num_threads();` / `omp_get_thread_num();`

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
* GPU向けにコンパイルするので, コンパイルオプションは `-mp=gpu` を使う
  * C++: `nvc++ -mp=gpu ...`
  * Fortran: `nvfortran -mp=gpu ...`
* 以下では `%%bash_submit` でジョブ投入して計算ノード(GPU搭載)上で実行する
  * 必要に応じて rscgrp や elapse などを指定して実行せよ
```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00
```

# `teams` $\sim$ チームの作成

* <font color="blue">構文</font>
  * C/C++:
```
#pragma omp target
#pragma omp teams
    S
```
  * Fortran:
```
!$omp target
!$omp teams
    文の並び
!$omp end teams
!$omp end target
```
* 多数のチームを作成し, 各チームの*マスター*が $S$ を実行する
* `parallel` に似ており, 多くのスレッドが同じ文を実行する効果を持つ
  * `teams` はチームを多数作る
  * `parallel` は各チーム内でスレッドを多数作る
* 実はマルチコアCPUでは初めからチームがひとつできており, そのマスターが `main` を実行するスレッドだったと考えればよい
* `omp_get_team_num()` / `omp_get_num_teams()` で, それぞれ自分のチーム番号と全チーム数が得られる (`omp_get_thread_num()` / `omp_get_num_threads()` のチーム版)

* 注: `teams` は `target` のすぐ内側に現れる必要がある. そのため `#pragma omp target teams` (Fortran: `!$omp target teams`) と一行(一指示)で書くことが多い

## C++版
"""

""" code w """
%%writefile omp_team_num.cpp
""" include nb/source/08_gpu_teams/include/omp_team_num.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_team_num.cpp -o omp_team_num_gpu.exe
""" """

""" md
* チーム数は以下のいずれかで指定できる
  * `teams` 構文に `num_teams(x)` を追加 (`parallel` の `num_threads` と類似)
  * 実行時に `OMP_NUM_TEAMS=x` 環境変数を設定 (`OMP_NUM_THREADS` と類似)
* 以下では `OMP_NUM_TEAMS=3` で実行する. 値を色々変えて実行してみよ
"""

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 ./omp_team_num_gpu.exe
""" """

""" md
## Fortran版
* Fortranでは `!$omp target` ... `!$omp end target`, `!$omp teams` ... `!$omp end teams` のように, 範囲を `end` 指示で明示する
"""

""" code w """
%%writefile omp_team_num.f90
""" include nb/source/08_gpu_teams/include/omp_team_num.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_team_num.f90 -o omp_team_num_f_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 ./omp_team_num_f_gpu.exe
""" """

""" md
* `OMP_NUM_TEAMS` を設定せずに実行してみよ
"""

""" code w """
%%bash_submit
./omp_team_num_gpu.exe
""" """

""" md
* おそらく108のチームが作られる
* それはこの環境のGPU (NVIDIA A100)に備わる「コア」の数である
* マルチコア環境で `OMP_NUM_THREADS` を指定せずに `parallel` を実行したときにコア数だけのスレッドが作られるのと似ている
* なおNVIDIA GPUでは普通のCPUで言うところの「コア」を<font color="blue">Streaming Multiprocessor</font>と呼ぶ
* つまり `OMP_NUM_TEAMS` や `num_teams` による指定を省略すると, (仕様にそう書かれているかは知らないが普通), Streaming Multiprocessorの数だけのチームが作られる
* WisteriaやこのGPU環境のNVIDIA A100は108のStreaming Multiprocessorを備えているということ
* なおこれは特段驚くような多さではなく, サーバー用のCPUでもこのくらいのコア数を持つものがある

# `distribute` $\sim$ for文の繰り返しをチーム間で分割実行

* <font color="blue">構文</font>
  * C/C++:
```
#pragma omp target teams distribute
    for (...) {
      ...
    }
```
  * Fortran:
```
!$omp target teams distribute
    do ...
      ...
    end do
!$omp end target teams distribute
```
* for文(Fortranでは do文)の繰り返しをチームに分配する
* だいたい
  * `teams` $\approx$ `parallel`
  * `distribute` $\approx$ `for`
* と思っておけばよい. `teams` と `distribute` の間に文がなければ `target teams distribute` のように一指示にまとめられる (`parallel for` と同じ)

## C++版
"""

""" code w """
%%writefile omp_target_teams_distribute.cpp
""" include nb/source/08_gpu_teams/include/omp_target_teams_distribute.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_target_teams_distribute.cpp -o omp_target_teams_distribute_gpu.exe
""" """

""" md
* 異なるチーム数と繰り返し回数 (コマンドライン引数 $m$) で実行し, どの繰り返しがどのチームに割り当てられるか観察せよ
"""

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 ./omp_target_teams_distribute_gpu.exe 7
""" """

""" md
## Fortran版
* ループ変数 `i` はチームごとに別の値を持つべきなので `private(i)` を付けている
"""

""" code w """
%%writefile omp_target_teams_distribute.f90
""" include nb/source/08_gpu_teams/include/omp_target_teams_distribute.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_target_teams_distribute.f90 -o omp_target_teams_distribute_f_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 ./omp_target_teams_distribute_f_gpu.exe 7
""" """

""" md
* 見ての通り `teams` と `distribute` だけで, `parallel` や `for` を使わずにループを並列化できる
* それで起こることはGPUの**各コア (Streaming Multiprocessor) で1つ**のスレッドが実行するということである
* CPUのコアはもともと1スレッドを実行するものだが, GPUのStreaming Multiprocessorは1つのコア内に多数のスレッドを実行する能力を持つ (逆に1スレッドの性能はCPUより遅い)
* `parallel` を使うと各チーム内にスレッドが作られ, 結果的に各Streaming Multiprocessor内で多数のスレッドを動かすことになる (次節)

# `parallel` $\sim$ チーム内でのスレッドの作成

* 典型的には `parallel` は `distribute` 内 (必然的に `teams` 内) で使う
* <font color="blue">構文</font>
  * C/C++:
```
#pragma omp target teams
    {
      ...
#pragma omp distribute
      for (...) {
        ...
#pragma omp parallel num_threads(x)
        S
      }
    }
```
  * Fortran:
```
!$omp target teams
    ...
!$omp distribute
    do ...
      ...
!$omp parallel num_threads(x)
      文の並び
!$omp end parallel
    end do
!$omp end distribute
!$omp end teams
```
* `teams` 内で `parallel` を使うと, **各チーム**内でスレッドが作られ, それぞれが $S$ を実行する

* <font color="red">重要な注意1: GPUでは `OMP_NUM_THREADS` は効き目がない模様</font>
  * CPUでは `parallel` のスレッド数を `OMP_NUM_THREADS=x` 環境変数や `num_threads(x)` で指定できた
  * しかしGPUで実行する場合は環境変数が効かない模様 (実装の問題か仕様の問題かは不明)
  * スレッド数を指定したい場合は `num_threads(x)` を使う必要がある
    * 以下のプログラムでは, 環境変数 `OMP_NUM_THREADS` を自分で読み取って (C++: `getenv`, Fortran: `get_environment_variable`) `num_threads(x)` に渡している
  * あるいは省略してシステムに任せる
* <font color="red">重要な注意2: スレッド数は32の倍数でなければならない模様</font>
  * GPUのハードウェアの仕組み (32スレッドからなる<font color="blue">warp</font>という単位で同時に実行する) を考えると頷ける
  * 32の倍数でない数を設定してもエラーは出ないので, くれぐれも誤った数を指定しないよう注意
  * 以下のプログラムは32の倍数でない `OMP_NUM_THREADS` を弾くようにしてある

## C++版
"""

""" code w """
%%writefile omp_distribute_parallel.cpp
""" include nb/source/08_gpu_teams/include/omp_distribute_parallel.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_distribute_parallel.cpp -o omp_distribute_parallel_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 OMP_NUM_THREADS=32 ./omp_distribute_parallel_gpu.exe 5
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile omp_distribute_parallel.f90
""" include nb/source/08_gpu_teams/include/omp_distribute_parallel.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_distribute_parallel.f90 -o omp_distribute_parallel_f_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 OMP_NUM_THREADS=32 ./omp_distribute_parallel_f_gpu.exe 5
""" """

""" md
# `for` (Fortran: `do`) $\sim$ for文の繰り返しをスレッド間で分割実行

* `parallel` 内で `for` (Fortranでは `do`) を使うと, ループの繰り返しを<font color="blue">チーム内のスレッド</font>に分配する
* これで4階層 (`teams` / `distribute` / `parallel` / `for`) が出揃う
  * 外側のループ `i` を `distribute` でチーム間に分割
  * 内側のループ `j` を `for` (`do`) で各チーム内のスレッド間に分割
* <font color="blue">構文</font>
  * C/C++:
```
#pragma omp target teams
#pragma omp distribute
    for (...) {            // i: チーム間に分割
#pragma omp parallel num_threads(x)
#pragma omp for
      for (...) {          // j: スレッド間に分割
        ...
      }
    }
```
  * Fortran:
```
!$omp target teams
!$omp distribute
    do ...                 ! i: チーム間に分割
!$omp parallel num_threads(x)
!$omp do
      do ...               ! j: スレッド間に分割
        ...
      end do
!$omp end do
!$omp end parallel
    end do
!$omp end distribute
!$omp end teams
```

## C++版
"""

""" code w """
%%writefile omp_for.cpp
""" include nb/source/08_gpu_teams/include/omp_for.cpp """
""" """

""" code w """
%%bash
nvc++ -mp=gpu omp_for.cpp -o omp_for_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 OMP_NUM_THREADS=32 ./omp_for_gpu.exe 5 6
""" """

""" md
## Fortran版
* 内側のループ変数 `j` は各スレッドで別の値を持つべきなので `parallel` に `private(j)` を付けている
"""

""" code w """
%%writefile omp_for.f90
""" include nb/source/08_gpu_teams/include/omp_for.f90 """
""" """

""" code w """
%%bash
nvfortran -mp=gpu omp_for.f90 -o omp_for_f_gpu.exe
""" """

""" code w """
%%bash_submit
OMP_NUM_TEAMS=3 OMP_NUM_THREADS=32 ./omp_for_f_gpu.exe 5 6
""" """

""" md
# よく出てくるディレクティブの結合

* 名目上は独立した4つの指示だが, 実際にはほとんど常に一緒に使われる
* 多くの場合, 目的はループを並列に実行することなので, 以下のいずれかの形で使われることが多い

1\. すべてを結合 (1重ループを一気にチーム×スレッドへ分割)
  * C/C++:
```
#pragma omp target teams distribute parallel for
    for (...) {
      ...
    }
```
  * Fortran:
```
!$omp target teams distribute parallel do
    do ...
      ...
    end do
!$omp end target teams distribute parallel do
```

2\. 外側のループを `teams` と `distribute` で, 内側のループを `parallel` と `for` (`do`) で並列化する (2重ループ)
  * C/C++:
```
#pragma omp target teams distribute
    for (...) {
#pragma omp parallel for
      for (...) {
        ...
      }
    }
```
  * Fortran:
```
!$omp target teams distribute
    do ...
!$omp parallel do
      do ...
        ...
      end do
!$omp end parallel do
    end do
!$omp end target teams distribute
```

* 1の形が最も手軽で, 単純な1重ループをGPUで並列化したいときの定番
* 2の形は, 外側・内側で別々の並列化レイヤーを割り当てたい (例えばデータの局所性を考えて使い分けたい) ときに使う
"""
