# 練習問題: モンテカルロ法で円周率 π を推定する (各スレッドが独立に)

## 目標

work-sharing 構文 (`for` / `do`) や `reduction` はまだ学んでいない. ここでは `omp_get_thread_num()` と `omp_get_num_threads()` だけを使って, モンテカルロ法による π の推定を複数スレッドで行う. 各スレッドは**自分の担当分の点を投げ, 自分自身の π 推定値を表示する**. 共有変数への足し込み (集約) はまだ行わない.

## 背景: モンテカルロ法

単位正方形 `[0,1) x [0,1)` 内に一様乱数で点を投げると, 原点中心・半径 1 の 1/4 円 (`x^2 + y^2 < 1`) の内側に落ちる確率は, その面積比 `π/4` に等しい. したがって

```
(円内に落ちた点の数) / (投げた点の総数) ≒ π/4
```

なので, この比を 4 倍すれば π の推定値が得られる. 点を多く投げるほど π ≒ 3.14159 に近づく.

## 課題

`montecarlo.cpp` (または `montecarlo.f90`) は, 全体で `N` 個 (コマンドライン引数, 既定 4,000,000) の点を投げる. 各スレッド `t` (全 `T` スレッド) は `N/T` 個の点を投げ, 円内に落ちた数を数えて自分の π 推定値を表示する.

コメント `TODO` の指示に従って **OpenMP の指示行を追加** し, このブロックを複数スレッドで実行させよ.

- C++: ブロック `{ ... }` の直前に `#pragma omp parallel` を1行加える. スレッドごとの変数 (`tid`, `hits` など) はブロック内で宣言してあるので, 自動的にスレッドごとに別々になる.
- Fortran: ブロックを `!$omp parallel private(...)` と `!$omp end parallel` で囲む. スレッドごとに別々にしたい変数を `private` 節に並べる.

**注意:** 1つの共有変数に全スレッドの `hits` を足し込んではならない (それは競合 (data race) になる. 総和をまとめる `reduction` は後のトピック). 各スレッドは**自分の**推定値だけを表示すること.

乱数は **状態を持たない (カウンタベースの) 純粋関数 `draw_rand01(seed, k)`** を使っている. 点 `i` の座標を `draw_rand01(i, 0)`, `draw_rand01(i, 1)` で決めるので, どのスレッドが計算しても点 `i` の位置は同じで, 共有状態が無いため競合も起きない. これは既に書かれているので変更不要. (同じ仕組みは `06_reduction/02_montecarlo_pi` や `14_montecarlo/00_gacha` でも使う.)

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore montecarlo.cpp -o montecarlo.exe

# Fortran
nvfortran -fast -mp=multicore montecarlo.f90 -o montecarlo.exe
```

```
OMP_NUM_THREADS=4 ./montecarlo.exe
OMP_NUM_THREADS=4 ./montecarlo.exe 40000000
```

## 期待される結果

`OMP_NUM_THREADS` に指定した数だけ行が表示され, 各スレッドが自分の推定値を出す (順番は実行ごとに入れ替わってよい). 各推定値は π ≒ 3.14159 に近い値になる. 例 (4スレッド):

```
thread 0 of 4: 1000000 points, pi estimate = 3.141234
thread 1 of 4: 1000000 points, pi estimate = 3.140876
thread 2 of 4: 1000000 points, pi estimate = 3.142560
thread 3 of 4: 1000000 points, pi estimate = 3.141100
```

投げる点の総数 `N` を増やすほど, 各推定値が π に近づくことを確認せよ. (穴あきのまま実行すると 1 スレッドだけが動き, 行が1つしか出ない.)
