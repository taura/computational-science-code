# 練習問題: 配列 (静的割り当て) の合計

## 目標

固定長の配列を添字でたどり, ループで全要素を集計する基本を身につける. C は添字 0 起点, Fortran は 1 起点であることに注意する.

## 課題

`array_static.cpp` (または `array_static.f90`) は, 配列 `a` を「(i+1) の二乗」(C++) / 「i の二乗」(Fortran) で埋めたあと, その合計を求める.
合計を計算するループ本体が空なので, 現状の合計は `0` のまま.

`TODO` の箇所に **配列の全要素を `s` に足し込むループ** を書け.

- C++: `for (int i = 0; i < n; i++) s += a[i];` (添字 0 〜 n-1)
- Fortran: `do i = 1, n` … `s = s + a(i)` … `end do` (添字 1 〜 n)

## コンパイルと実行

```
# C++
nvc++ -fast array_static.cpp -o array_static.exe

# Fortran
nvfortran -fast array_static.f90 -o array_static.exe
```

```
./array_static.exe
```

## 期待される結果

1²+2²+…+10² = 385 なので

```
sum of squares 1..10 = 385
```

ループを書く前は `0` になる (集計できていないことの確認).
