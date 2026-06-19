# 練習問題: モンテカルロ法で π を求める (reduction)

## 目標

`reduction(+:count)` 句を1つ追加するだけで, 複数のスレッドが同じカウンタ `count` を同時に増やそうとして起こる競合 (data race) を解消し,
モンテカルロ法による円周率 π の推定を正しく並列化できることを体験する.

## 課題

`montecarlo_pi.cpp` (または `montecarlo_pi.f90`) は, 単位正方形 [0,1)×[0,1) に n 個の点をランダムに投げ,
原点からの距離が 1 未満 (= 半径 1 の円の 1/4 の内側) に入った点の数 `count` を数えて, π ≈ 4 * count / n を求めるコードである.
乱数は反復番号から決まる線形合同法 (LCG) で生成しているので, スレッド数によらず同じ点列になる.

このループを単純に並列化すると, 全スレッドが共有変数 `count` に同時に `count++` を行うため競合 (data race) が起き,
スレッド数が2以上だと**数え落とし**が発生して π が小さめに出たり, 実行ごとに値が変わったりする.

コメント `TODO` の箇所で **`reduction` を用いて並列化せよ**. 各スレッドが部分カウントを持ち寄って正しい合計を得るようになる.

- C++: `#pragma omp parallel for reduction(+:count)`
- Fortran: `!$omp parallel do private(x, y) reduction(+:count)` … `!$omp end parallel do`

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore montecarlo_pi.cpp -o montecarlo_pi.exe

# Fortran
nvfortran -fast -mp=multicore montecarlo_pi.f90 -o montecarlo_pi.exe
```

```
OMP_NUM_THREADS=4 ./montecarlo_pi.exe
```

第1引数で点数 `n` を変えられる (例: `./montecarlo_pi.exe 1000000000`).

## 期待される結果

点数 n を大きくするほど, π の推定値は真の値 3.14159... に近づく.

```
pi ~= 3.141...
```

- `reduction(+:count)` を**追加すると**, スレッド数によらず常に同じ正しい値になる.
- 参考: もし `reduction` を付けずに共有変数 `count` をそのまま `count++` で並列に更新すると (例: `OMP_NUM_THREADS=4`),
  複数スレッドの加算がぶつかって数え落としが起き, `count` が本来より小さくなって π が 3.14 より小さく出たり, 実行ごとに値が変わったりする.
  これが reduction が必要な理由である.
