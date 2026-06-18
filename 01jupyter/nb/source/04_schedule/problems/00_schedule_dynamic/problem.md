# 練習問題: schedule(dynamic) で負荷を均す

## 目標

すでに並列化されたループに `schedule(dynamic)` 句を1つ追加するだけで, 繰り返しごとの仕事量が大きく異なる (アンバランスな) ループの負荷をスレッド間で均等に分担できることを体験する.

## 課題

`schedule_dynamic.cpp` (または `schedule_dynamic.f90`) のループは, 各繰り返し `i` の仕事量が `i` に比例して増える (内側ループが `i` 回まわる) ため, デフォルトの分割 (static) では, 大きい `i` を割り当てられたスレッドだけが長く働き, 他のスレッドが待たされる.

コメント `TODO` の箇所で **ループを並列化し, `schedule(dynamic)` で負荷分散せよ**.

- C++: `#pragma omp parallel for schedule(dynamic) reduction(+:total)`
- Fortran: `!$omp parallel do schedule(dynamic) reduction(+:total)` … `!$omp end parallel do`

それ以外のコードを変更する必要はない.

## コンパイルと実行

```
# C++
nvc++ -fast -mp=multicore schedule_dynamic.cpp -o schedule_dynamic.exe

# Fortran
nvfortran -fast -mp=multicore schedule_dynamic.f90 -o schedule_dynamic.exe
```

`schedule(dynamic)` を入れる前と後で, 実行時間を `time` で比較せよ.

```
OMP_NUM_THREADS=4 time ./schedule_dynamic.exe
```

## 期待される結果

- 計算結果 `total` は schedule の指定に関わらず同じ.
- デフォルト (static) では一部のスレッドに重い繰り返しが偏り, 実行時間が長くなる.
- `schedule(dynamic)` を追加すると, 空いたスレッドが順に繰り返しを取りに行くので負荷が均され, 実行時間が短くなる.

`OMP_NUM_THREADS` の値を変えて, 短縮の効果を確認せよ.
