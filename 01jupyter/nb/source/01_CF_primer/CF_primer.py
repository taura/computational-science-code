""" md

#* C++ / Fortran 入門 --- 数値計算コードを読み書きする

# このトピックのねらい

* これ以降のトピックで OpenMP による並列化を学ぶ前提として, C++ と Fortran で書かれた数値計算コードを **読み書きできる** ことを目標とする
* 全くの未経験者向けの丁寧すぎる説明はしない. 他の言語(Python など)の経験があることを前提に, 「同じことを C/C++ ではこう書く, Fortran ではこう書く」という対比で要点を押さえる
* 扱う題材
  * hello world (コンパイルと実行)
  * 数値型
  * 関数定義と呼び出し
  * 配列 (静的割り当て・動的割り当て)
  * ループ (for/do 文, while 文)
  * ファイル入出力の基本
  * コマンドライン引数処理の基本

# C/C++ と Fortran --- おおまかな違い

* どちらもコンパイル型の言語. ソースを **コンパイル** して実行可能ファイルを作り, それを実行する
* C++ (拡張子 `.cpp`)
  * 文はセミコロン `;` で終わる. ブロックは `{{ ... }}` で囲む
  * 添字は **0 から** 始まる (`a[0]` 〜 `a[n-1]`)
  * 大文字小文字を区別する
* Fortran (拡張子 `.f90`)
  * 1文1行が基本 (継続は行末に `&`). ブロックは `end ...` で閉じる
  * 配列添字は既定で **1 から** 始まる (`a(1)` 〜 `a(n)`), 配列アクセスは `()` で書く
  * 大文字小文字を区別しない
  * 数値計算向けの機能(配列演算, 複素数など)が言語に組み込まれている

# 環境設定

* コンパイラを使えるようにする (GPUは使わないが他のトピックと同じ設定でよい)
"""

""" code w """
import os
paths = os.environ["PATH"].split(":")
nvc_path = "/work/opt/local/x86_64/cores/nvidia/23.3/Linux_x86_64/23.3/compilers/bin"
fj_path = "/opt/FJSVxtclanga/tcsds-1.2.41/bin"
for path in [nvc_path, fj_path]:
    if path not in paths:
        paths = [path] + paths
os.environ["PATH"] = ":".join(paths)
""" """

""" code w """
%%bash
which nvc++
which nvfortran
""" """

""" md
# hello world (コンパイルと実行)

* まずは画面に文字列を表示するだけのプログラム
* C++ では `main` 関数がプログラムの入り口. `#include <cstdio>` で `printf` が使えるようになる
* Fortran では `program 名前` 〜 `end program 名前` がプログラム本体

## C++版
"""

""" code w """
%%writefile hello.cpp
""" include nb/source/01_CF_primer/include/hello.cpp """
""" """

""" md
* コンパイル. `-o` で出力する実行可能ファイル名を指定する. `-fast` は最適化オプション
"""

""" code w """
%%bash
nvc++ -fast hello.cpp -o hello_cpp.exe
""" """

""" code w """
%%bash
./hello_cpp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile hello.f90
""" include nb/source/01_CF_primer/include/hello.f90 """
""" """

""" code w """
%%bash
nvfortran -fast hello.f90 -o hello_f.exe
""" """

""" code w """
%%bash
./hello_f.exe
""" """

""" md
* 参考: Odyssey (富士通コンパイラ) では `FCCpx hello.cpp -o hello_cpp.exe` (C++), `frtpx hello.f90 -o hello_f.exe` (Fortran)
* `print "(a)", ...` の `"(a)"` は出力の書式(format)で, `a` は文字列を意味する. 数値の書式はこの後出てくる

# 数値型

* 数値計算では, 整数と浮動小数点(小数), およびその精度(ビット数)を意識することが重要
* C++ の主な型: `int` (整数, 普通32bit), `long` (長い整数, 普通64bit), `float` (単精度32bit), `double` (倍精度64bit)
* Fortran では `integer(4)`, `integer(8)`, `real(4)`, `real(8)` のように **バイト数**で精度を指定するのが分かりやすい (`real(8)` が C の `double` に相当)
* 共通の注意点: **整数同士の割り算は小数部が切り捨てられる** (`7 / 2` は `3`). 小数で割りたければ少なくとも一方を浮動小数点にする

## C++版
"""

""" code w """
%%writefile types.cpp
""" include nb/source/01_CF_primer/include/types.cpp """
""" """

""" code w """
%%bash
nvc++ -fast types.cpp -o types_cpp.exe && ./types_cpp.exe
""" """

""" md
* `%d` `%ld` `%f` などは `printf` の書式指定子で, それぞれ `int`, `long`, 浮動小数点に対応する
* Fortran では倍精度の定数を `3.14d0` のように `d0` を付けて書く (付けないと単精度扱いになり精度が落ちる)

## Fortran版
"""

""" code w """
%%writefile types.f90
""" include nb/source/01_CF_primer/include/types.f90 """
""" """

""" code w """
%%bash
nvfortran -fast types.f90 -o types_f.exe && ./types_f.exe
""" """

""" md
* `print` の書式 `(i0)` は整数(幅は自動), `(f0.6)` は小数点以下6桁の浮動小数点を表す

# 関数定義と呼び出し

* 処理をまとめて名前を付け, 引数を渡して呼び出すのが関数
* C++: `返り値の型 関数名(引数の型 引数名, ...) {{ ... return 値; }}`
* Fortran: `function 関数名(引数) result(返り値変数)` 〜 `end function`. 複数の関数は `module` にまとめ, 使う側で `use` するのが現代的な書き方
  * 引数には `intent(in)` (入力専用) などを付けると意図が明確になる

## C++版
"""

""" code w """
%%writefile func.cpp
""" include nb/source/01_CF_primer/include/func.cpp """
""" """

""" code w """
%%bash
nvc++ -fast func.cpp -o func_cpp.exe && ./func_cpp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile func.f90
""" include nb/source/01_CF_primer/include/func.f90 """
""" """

""" code w """
%%bash
nvfortran -fast func.f90 -o func_f.exe && ./func_f.exe
""" """

""" md
# 配列 (静的割り当て)

* 同じ型の値を並べて添字でアクセスするのが配列. 数値計算の主役
* 「静的割り当て」= 要素数をコンパイル時に決める配列
* C++: `double a[n];` (添字 0〜n-1). `n` はコンパイル時に決まる定数であること
* Fortran: `real(8) :: a(n)` (添字 1〜n). 要素数は `parameter` 定数で与える
* <font color="red">添字の起点が C は 0, Fortran は 1</font> という違いに常に注意

## C++版
"""

""" code w """
%%writefile array_static.cpp
""" include nb/source/01_CF_primer/include/array_static.cpp """
""" """

""" code w """
%%bash
nvc++ -fast array_static.cpp -o array_static_cpp.exe && ./array_static_cpp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile array_static.f90
""" include nb/source/01_CF_primer/include/array_static.f90 """
""" """

""" code w """
%%bash
nvfortran -fast array_static.f90 -o array_static_f.exe && ./array_static_f.exe
""" """

""" md
# 配列 (動的割り当て)

* 要素数を **実行時に** 決めたい場合 (例: コマンドライン引数で問題サイズを与える) は動的割り当てを使う
* C++: `double * a = (double *)malloc(sizeof(double) * n);` で確保し, 使い終わったら `free(a);` で解放する
  * `double *` は「`double` を指すポインタ」型. 配列の先頭アドレスを保持する. `a[i]` で要素にアクセスできる
* Fortran: `real(8), allocatable :: a(:)` と宣言し, `allocate(a(n))` で確保, `deallocate(a)` で解放する. C より簡潔で安全

## C++版
"""

""" code w """
%%writefile array_dynamic.cpp
""" include nb/source/01_CF_primer/include/array_dynamic.cpp """
""" """

""" code w """
%%bash
nvc++ -fast array_dynamic.cpp -o array_dynamic_cpp.exe && ./array_dynamic_cpp.exe 100
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile array_dynamic.f90
""" include nb/source/01_CF_primer/include/array_dynamic.f90 """
""" """

""" code w """
%%bash
nvfortran -fast array_dynamic.f90 -o array_dynamic_f.exe && ./array_dynamic_f.exe 100
""" """

""" md
# ループ (for / do 文, while 文)

* 繰り返しはループで書く. 数値計算では配列の各要素を順に処理するのに多用する
* C++: `for (初期化; 条件; 更新) {{ ... }}`, 条件で繰り返す `while (条件) {{ ... }}`
* Fortran: `do 変数 = 始点, 終点 [, 増分]` 〜 `end do`, 条件で繰り返す `do while (条件)` 〜 `end do`
* (これ以降のトピックで OpenMP が並列化するのは, 主にこの `for`/`do` ループである)

## C++版
"""

""" code w """
%%writefile loop.cpp
""" include nb/source/01_CF_primer/include/loop.cpp """
""" """

""" code w """
%%bash
nvc++ -fast loop.cpp -o loop_cpp.exe && ./loop_cpp.exe
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile loop.f90
""" include nb/source/01_CF_primer/include/loop.f90 """
""" """

""" code w """
%%bash
nvfortran -fast loop.f90 -o loop_f.exe && ./loop_f.exe
""" """

""" md
# ファイル入出力の基本

* 計算結果をファイルに保存したり, 入力データを読み込んだりする
* C++: `fopen` で開き (`"w"`=書き込み, `"r"`=読み込み), `fprintf`/`fscanf` で書き/読み, `fclose` で閉じる
* Fortran: `open(unit=番号, file=..., ...)` で開き, `write`/`read` で書き/読み, `close` で閉じる. 読み込みの終端は `iostat` で判定する

## C++版
"""

""" code w """
%%writefile fileio.cpp
""" include nb/source/01_CF_primer/include/fileio.cpp """
""" """

""" code w """
%%bash
nvc++ -fast fileio.cpp -o fileio_cpp.exe && ./fileio_cpp.exe && echo "--- data.txt ---" && cat data.txt
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile fileio.f90
""" include nb/source/01_CF_primer/include/fileio.f90 """
""" """

""" code w """
%%bash
nvfortran -fast fileio.f90 -o fileio_f.exe && ./fileio_f.exe && echo "--- data.txt ---" && cat data.txt
""" """

""" md
# コマンドライン引数処理の基本

* 問題サイズやパラメータを, 実行時にコマンドラインから渡せると便利 (これまでのトピックでも `./a.exe 72 100000000` のように使ってきた)
* C++: `int main(int argc, char ** argv)` の `argc` が引数の個数(プログラム名を含む), `argv[i]` が i 番目の引数(文字列). `atoi`/`atof` で数値に変換する
* Fortran: `command_argument_count()` が引数の個数(プログラム名を含まない), `get_command_argument(i, 変数)` で i 番目を取り出す (i=0 はプログラム名). 数値変換は内部 `read` で行う

## C++版
"""

""" code w """
%%writefile args.cpp
""" include nb/source/01_CF_primer/include/args.cpp """
""" """

""" code w """
%%bash
nvc++ -fast args.cpp -o args_cpp.exe && ./args_cpp.exe 5 2.5
""" """

""" md
## Fortran版
"""

""" code w """
%%writefile args.f90
""" include nb/source/01_CF_primer/include/args.f90 """
""" """

""" code w """
%%bash
nvfortran -fast args.f90 -o args_f.exe && ./args_f.exe 5 2.5
""" """

""" md
# まとめ

* これで, このあとのトピックで出てくる数値計算コードを読み書きするのに必要な C++ / Fortran の基本要素が一通り揃った
* 特に重要な対比
  * 配列添字: C は 0 起点, Fortran は 1 起点
  * 動的配列: C は `malloc`/`free` + ポインタ, Fortran は `allocatable` + `allocate`/`deallocate`
  * 並列化の主対象となるループ: C の `for`, Fortran の `do`
* 次のトピックからは, これらのループを OpenMP で並列化していく
"""
