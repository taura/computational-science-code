# 練習問題: teams / distribute / parallel / for の出力行数を予測する

## 目標

`teams`, `distribute`, `parallel`, `for` を入れ子にしたとき, それぞれの領域が「何回」実行されるかを正しく理解する.
この問題は**コードを書く問題ではなく, 出力行数を予測してから実測で確かめる読解・予測の問題**である.

## プログラムの構造

`teams_quiz.cpp` (または `teams_quiz.f90`) は完成済みで, 次の入れ子構造を持つ.

```
#pragma omp target teams            // T 個のチームを作る
  in teams を表示
  #pragma omp distribute            // i = 0 .. m-1 をチーム間で分配
  for i in 0..m-1
    in distribute を表示
    #pragma omp parallel num_threads(H)   // 各チーム内に H 本のスレッド
      in parallel を表示
    #pragma omp for                 // j = 0 .. n-1 をスレッド間で分配
    for j in 0..n-1
      in for を表示
```

- チーム数は `OMP_NUM_TEAMS` で指定する (これを $T$ とする).
- 1チームあたりのスレッド数は `OMP_NUM_THREADS` で指定する (これを $H$ とする). `num_threads()` 経由でプログラムに渡している.
  - `OMP_NUM_THREADS` は **1 または 32 の倍数** でなければならない (GPUのスレッドは32本単位(ワープ)で動くため). それ以外を指定するとプログラムがエラーで止まる.
- コマンドライン引数 `m`, `n` がループ回数になる.

## 課題

`OMP_NUM_TEAMS=T`, `OMP_NUM_THREADS=H`, 引数 `m`, `n` で実行したとき, 次の各メッセージが何行表示されるかを **$T$, $H$, $m$, $n$ の式で予測** せよ.

- `in teams:` の行数
- `in distribute:` の行数
- `in parallel:` の行数
- `in for:` の行数
- 合計行数

ヒント: `in teams` はチームごとに1回. `distribute` と `for` はループの繰り返しを分配する (繰り返しの総数だけ表示される) のに対し, `parallel` は各チーム内でスレッドを作る (チーム内のスレッドの数だけ表示される) ことに注意せよ.

## コンパイルと実行

```
# C++
nvc++ -mp=gpu teams_quiz.cpp -o teams_quiz.exe

# Fortran
nvfortran -mp=gpu teams_quiz.f90 -o teams_quiz.exe
```

GPUは計算ノードにのみ搭載されているので, `%%bash_submit` でジョブとして投入して実行する.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6
```

## 予測の確認

`wc -l` で実際の行数を数え, 予測と一致するか確かめよ. メッセージごとに `grep` で絞り込むと数えやすい.

```
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:10:00

OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6 | wc -l
OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6 | grep "in teams"      | wc -l
OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6 | grep "in distribute" | wc -l
OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6 | grep "in parallel"   | wc -l
OMP_NUM_TEAMS=2 OMP_NUM_THREADS=32 ./teams_quiz.exe 5 6 | grep "in for"        | wc -l
```

$T$, $H$, $m$, $n$ をいろいろ変えて実行し, 予測した式が常に成り立つことを確認せよ.
表示される行の `team` 番号や `thread` 番号にも注目すると, どの繰り返しがどのチーム・スレッドに割り当てられたかが分かる.
