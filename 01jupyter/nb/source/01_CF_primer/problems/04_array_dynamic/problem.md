# 練習問題: 配列 (動的割り当て)

## 目標

要素数を **実行時に** 決める動的配列の確保のしかたを身につける.
C++ は `malloc`/`free` とポインタ, Fortran は `allocatable` と `allocate`/`deallocate` を使う.

## 課題

`array_dynamic.cpp` (または `array_dynamic.f90`) は, 要素数 `n` をコマンドライン引数で受け取り, `1/1 + 1/2 + … + 1/n` を計算する.
配列の確保 (メモリの割り当て) が抜けているので, このままでは正しく動かない (実行時にエラーになる).

`TODO` の箇所に **配列を確保する処理** を書け.

- C++: `a = (double *)malloc(sizeof(double) * n);`
- Fortran: `allocate(a(n))`

(解放 `free(a)` / `deallocate(a)` は最後にすでに書いてある.)

## コンパイルと実行

```
# C++
nvc++ -fast array_dynamic.cpp -o array_dynamic.exe

# Fortran
nvfortran -fast array_dynamic.f90 -o array_dynamic.exe
```

```
./array_dynamic.exe 100
./array_dynamic.exe 1000000
```

## 期待される結果

`n` を大きくするほど合計はゆっくり増える (調和級数). 例:

```
sum of 1/k (k=1..100) = 5.187378
```

確保を書く前に実行すると, 異常終了する (メモリを確保していないため). 引数 `n` を変えて値が変わることも確かめよ.
