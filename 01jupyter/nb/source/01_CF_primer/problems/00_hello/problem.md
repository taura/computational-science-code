# 練習問題: hello world (コンパイルと実行)

## 目標

ソースを書き, コンパイルして実行する一連の流れを確認する. 文字列の表示と改行の書き方に慣れる.

## 課題

`hello.cpp` (または `hello.f90`) の `TODO` の箇所に表示文を書き, 次の2行をちょうど表示させよ.

```
hello, world
I am learning C++ and Fortran
```

- C++: `printf("...\n");` を2つ書く. 行末の改行 `\n` を忘れないこと.
- Fortran: `print "(a)", "..."` を2つ書く (Fortran は1文ごとに自動で改行される).

## コンパイルと実行

```
# C++
nvc++ -fast hello.cpp -o hello.exe

# Fortran
nvfortran -fast hello.f90 -o hello.exe
```

```
./hello.exe
```

## 期待される結果

```
hello, world
I am learning C++ and Fortran
```
