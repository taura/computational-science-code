# 練習問題: ファイル入出力

## 目標

ファイルを開いて読み込み, 終端まで繰り返し読むループの書き方を身につける.

## 課題

`fileio.cpp` (または `fileio.f90`) は, まず `data.txt` に 5 行の数値 (`i` と `i*0.5`) を書き出す (この部分は完成済み).
そのあと `data.txt` を読み直して, 2列目 `x` の合計を求めたい.
読み込みループが空なので, 現状の合計は `0`.

`TODO` の箇所に **1行ずつ読み, 読めた間 `x` を `sum` に足し込むループ** を書け.

- C++: `while (fscanf(rp, "%d %lf", &i, &x) == 2) { sum += x; }` (2個読めた間繰り返す)
- Fortran: `do` … `read(10, *, iostat=ios) i, x` … `if (ios /= 0) exit` … `sum = sum + x` … `end do` (`iostat` が 0 でなくなったら終端)

## コンパイルと実行

```
# C++
nvc++ -fast fileio.cpp -o fileio.exe

# Fortran
nvfortran -fast fileio.f90 -o fileio.exe
```

```
./fileio.exe
cat data.txt   # 書き出された中身を確認
```

## 期待される結果

`x` は `0.0, 0.5, 1.0, 1.5, 2.0` なので合計は 5.0.

```
sum of x = 5.000000
```

読み込みループを書く前は `0` になる. `data.txt` の中身も `cat` で確認せよ.
