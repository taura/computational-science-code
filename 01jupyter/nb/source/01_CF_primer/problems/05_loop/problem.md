# 練習問題: ループ (while / do while)

## 目標

回数が事前に決まっていない繰り返しを, 条件で続ける `while` (Fortran は `do while`) で書けるようになる.

## 課題

`loop.cpp` (または `loop.f90`) は, 与えられた `N` に対して **2 を何回かけたら `N` を超えるか** (`2^k > N` となる最小の `k`) を求める.
繰り返しの本体が空なので, 現状では `k = 0`, `p = 1` のまま.

`TODO` の箇所に **`p` が `N` を超えるまで「`p` を 2 倍し, `k` を 1 増やす」を繰り返す** ループを書け.

- C++: `while (p <= N) { p = p * 2; k++; }`
- Fortran: `do while (p <= N)` … `p = p * 2` … `k = k + 1` … `end do`

## コンパイルと実行

```
# C++
nvc++ -fast loop.cpp -o loop.exe

# Fortran
nvfortran -fast loop.f90 -o loop.exe
```

```
./loop.exe 1000
./loop.exe 1000000
```

## 期待される結果

`2^10 = 1024` が 1000 を超える最初の 2 のべきなので

```
2^10 = 1024 is the first power of 2 greater than 1000
```

ループを書く前は `2^0 = 1` のままになる. 引数 `N` を変えて結果が変わることを確かめよ.
