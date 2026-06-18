# 目標

- OpenMP を使った高性能プログラミングを教える教材を作る
- 授業は計算科学概論だが、データ処理やAIも範疇に入る
- ただし情報系の学生の授業ではないのでプログラミングの経験値は低いことを前提とする
- 範囲
	- OpenMPによるマルチコアプログラミング (parallel, for, schedule, data directives)
	- GPU オフローディング (target, teams, distribute, parallel, for)
	- SIMD (OpenMP simd directive, Intel SIMD intrinsics)
	- CPU instruction level parallelism (ILP)
	- 言語は C++ および Fortranが対象
- 以下にこれまで作ったものがある。これらは自動スクリプトで変換されて Jupyter Notebook 形式で学生に提供される
	- `cs01/cs01_omp_multicore.py`
	- `cs02/cs02_omp_gpu.py`
	- `cs03/cs03_omp_integral.py`
	- `cs04/cs04_simd.py`
	- `cs05/cs05_omp_integral_simd_ilp_mp.py`
- 上記は、到達目標、どんなコンパイラを使うか、学生に使わせるツール (bash_submit, AIチューターhey) などの詳細を知るために使って下さい。最終的な成果物はこれとは大きく異なる予定
- 特に, `cs03/cs03_omp_integral.py` `cs05/cs05_omp_integral_simd_ilp_mp.py` は練習問題なのでトピックとしては使わない

# 成果物イメージ

- 各トピックは、このディレクトリ (`nb/source`) の直下に、トピック順の番号を付けたフォルダとして置く (例: `01_intro`, `02_parallel`, ..., `13_ilp`)。`output` フォルダは使わない
- トピックごとにひとつ .py ファイルがある (フォルダ内に置く。例: `02_parallel/parallel.py`)。ファイルの形式は上記の .py ファイルから推論せよ。ほとんどがコメントで、""" md ... """ が本文。 """ code """ ... """ """ がコードセルとなって Jupyter で実行させる。
- トピックの粒度は今より少し細かいものをイメージしている。以下が実際のフォルダ構成 (トピック順)
  - `01_intro` --- C++, Fortran 入門
  - `02_parallel` --- parallel, `omp_get_num_threads()`, `omp_get_thread_num()`
  - `03_for_collapse` --- for, collapse
  - `04_schedule` --- schedule
  - `05_speedup` --- 台数効果の測定
  - `06_reduction` --- data directives, 特に reduction
  - `07_gpu_target` --- GPU target
  - `08_gpu_teams` --- GPU teams, distribute, parallel, for
  - `09_gpu_map` --- GPU データ移動 (map directive)
  - `10_gpu_speedup` --- GPU 上での台数効果測定
  - `11_simd` --- SIMD (SIMD directive)
  - `12_simd_intrinsics` --- SIMD intrinsics
  - `13_ilp` --- ILP
- 各トピックフォルダの構成
  - トップレベルの .py ファイル (そのトピックを説明する本文)
  - `include/` --- .py から `""" include nb/source/<NN_topic>/include/<file> """` で取り込むソース (C++ の `.cpp` と Fortran の `.f90`)。include のパスは `nb/source/` から始まる絶対的な指定なので、フォルダ名を変えたら .py 内のパスも合わせて直すこと
  - 練習問題は `problems/` サブディレクトリの下に置く。問題ごとにひとつのディレクトリを作り、トピック内の出題順に `NN_問題名` と番号を付ける (番号は `00` から開始。例: `02_parallel/problems/00_hello_threads/`)。その中に問題を記述した markdown (`problem.md`) と、逐次/穴あきのベースラインコード (C++ と Fortran) を置く
- 各トピックについて, トップレベルのファイルと, その事項に関連する練習問題を複数作る。練習問題はそこまで知っている内容だけで何かが動く、というもの。基本、逐次のベースラインコードを与えて、それをOpenMPで並列化・GPU化・SIMD化する作業をさせる
  - トップレベルのファイルを作るにあたっては上に上げた .py ファイルからコピペするのことを基本にする（あまり独創しない）
	- ただし、Fortranについての説明も付け加える（Cでの説明をFortranの説明に置き換える）
  - 練習問題のmarkdownは、問題を明確に記述しつつ、自己完結した簡潔なファイルにする
  - ジョブ投入の `#PJM -L rscgrp=...` は `rscgrp=lecture-a` を使う (元の cs0X ファイルの `lecture7-a` は使わない)
  - また、C++, Fortran 入門のトピックはこれまでの教材はないので新たいつ来る。同じことをC/C++ではどう書く、Fortranではどう書く、という対比を重んじる。練習問題として以下をカバーする。数値計算コードが読み書きできるレベルが到達点。全くの未経験者向けの丁寧すぎる説明は不要。
	- hello world (コンパイルと実行)
	- 数値型
	- 関数定義と呼び出し
	- 配列（静的割り当て・動的割り当て）
	- ループ（for/do 文、while文）
	- ファイル入出力の基本
	- コマンドライン引数処理の基本

		
