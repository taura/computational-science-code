""" md
#* 高性能プログラミングと性能測定(0) --- Jupyter演習環境
"""

""" md

# このnotebookの使い方 / How this notebook works

* スーパーコンピュータ(Wisteria)で, SSH + コマンドの代わりに, プログラミングを学習する環境
* 本演習では
  * Wisteria Aquarius (Intel CPU + NVIDIA GPU) を用いる
  * マルチコア並列 (OpenMP) の演習では Wisteria Odyssey を用いることもできる
* Jupyterのセルを実行するだけで例題プログラムを保存, コンパイル, 実行できる
* GPU (NVIDIA GPUが必要), Odyssey (CPUがログインノードと異なる) を用いず, かつすぐに終了する (ログインノードにほとんど負荷をかけない) コマンドであればログインノード (Intel CPU) で直接実行
* それ以外の場合はセルをほんの少し修正するとそれをバッチキューに入れて実行できる

* 入口: https://wisteria08.cc.u-tokyo.ac.jp:8000/jupyterhub/ 
* オリジナルドキュメント: [Wisteria利用支援ポータル](https://wisteria-www.cc.u-tokyo.ac.jp/cgi-bin/hpcportal.ja/index.cgi) にログイン -> ドキュメント閲覧 -> Wisteria/BDEC-01 システム利用手引書 -> 2.7節 JupyterHub

# Jupyterが始めての人へのJupyter 入門 / Introduction to Jupyter for beginners

## セル / Cell

* 以下のような入力欄を「セル」という
* SHIFT+ENTERで実行できる
<br/>

* A textbox like below is called a "cell"
* Press SHIFT+ENTER to execute it

## Python

"""

""" code w """
def f(x):
  return x + 1

f(3)
""" """

""" md 
* 実行中のセルは, 左に`[*]`と表示され, 終了すると`[2]`のような番号に変わる
* `[*]`が表示されている間は他のセルを実行できないことを覚えておこう
* 以下のセルは3秒間sleep (休眠)するプログラム
* 実行すると3秒間 `[*]` ｔなることを観察しておくこと
<br/>

* while a cell is executing, `[*]` is shown on the left, which turns into a number like `[2]`
* remember that you cannot execute other cells while `[*]` is shown
* the cell below sleeps for 3 seconds
* execute it and observe `[*]` is shown for 3 seconds

"""

""" code w """
import time
time.sleep(3.0)
""" """

""" md 
* 実行中のセルを途中で止めたければタブ上部の■ボタンで止められることにはなっているが効かないことも多い
* その場合は, メニューの Kernel -&gt; Restart Kernel として **カーネルのリセット** をする
* より強力なリセット方法はメニューの File -&gt; Hub Control Panel -&gt; Stop Server ->&gt; Start Server として**サーバの再起動** をする
* 以下を実行し, 終了する前(5秒以内)に■ボタンで止めてみよ
* カーネルのリセット, サーバの再起動も試してみよ
<br/>

* you should be able to stop an executing cell by ■ button at top of the tab, but do not expect it to work reliably
* if it doesn't work, **reset kernel** by going to menu and selecting Kernel -&gt; Restart Kernel
* even more powerful method to reset everything is to **restart the server** by going to menu and selecting File -&gt; Hub Control Panel -&gt; Stop Server -&gt; Start Server
* execute the cell below and stop it before it finishes by ■ button
* also try to reset kernel and restart server
"""

""" code w """
import time
time.sleep(5.0)
""" """

""" md 

* 以下のように
```
%%writefile ファイル名
```
で始まるセルは SHIFT + Enter で実行すると中身が指定された「ファイル名」に保存される(だけ)という効果を持つ.
実際にプログラムとして実行されるわけではないので中身は何でも良い(Pythonプログラムでなくても良い).

"""

""" code w """
%%writefile hello.c

#include <stdio.h>
int main() {
  printf("hello\n");
  return 0;
}
""" """

""" md 
## bash

* セルの先頭に`%%bash` と書くとセルの内容はシェルコマンド (Linuxのコマンド) の意味になる
<br/>

* If you put `%%bash` in the beginning of a cell, the content of the cell is read as shell commands

"""

""" code w """
%%bash
pwd
""" """

""" code w """
%%bash
ls
""" """

""" md 
* 上記で `hello.c` として保存されたはずのプログラムを下記でコンパイル・実行
"""

""" code w """
%%bash
gcc -o hello hello.c
""" """

""" code w """
%%bash
./hello
""" """

""" md 
## text (markdown)

* コードではなくテキスト(マークダウン形式)を書くためのセル
* there are cells for ordinary texts (markdown format), not code

"""

""" md w 
* ここをダブルクリックして編集してみよ
  * 編集し終えたらSHIFT-ENTERで保存
* double-click this cell and edit
  * after done, press SHIFT-ENTER to save
"""

""" md
## セルの追加, 削除

* Editメニューからあるセルを選択して
  * `a` : 上にセルを追加
  * `b` : 下にセルを追加
  * `x` : セルを削除
することができるので, 自分でコードを書いて試してみたい, AIに質問をしたい場合 (詳細以下) などに適宜利用されたい

"""

""" md

# 本環境の拡張機能1: AIチューター

* 安易に何でもAIに聞いて済ませるのがよいわけではないが, あらゆる角度から質問でき, すぐに答えが返ってくるのがAIの利点なので活用されたい
  * よくない質問の仕方: 「この(演習)問題の答え教えて」
  * よい質問の仕方: 「◯◯について基本から教えて」「C言語で ... は何のこと?」etc.

## 設定

"""

""" code """
import heytutor
""" """

""" md

## 質問例

"""

""" code w """
%%hey

C言語でコマンドライン引数を受け取る方法を教えて
""" """

""" code w """
%%hey

以下のプログラムで正しい答えが出ないんだけどなぜ?

#include <stdio.h>
#include <stdlib.h>

double int_inv_1_x2(double a, double b, long n) {
  double s = 0.0;
  double dx = (b - a) / (double)n;
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    double x = a + i * dx;
    s += 1 / (1 + x * x);
  }
  return s * dx;
}

int main(int argc, char ** argv) {
  double a = (argc > 1 ? atof(argv[1]) : 0.0);
  double b = (argc > 2 ? atof(argv[2]) : 1.0);
  long n   = (argc > 3 ? atol(argv[3]) : 1000L * 1000L * 1000L);
  double s = int_inv_1_x2(a, b, n);
  printf("s = %f\n", s);
  return 0;
}

""" """

""" code w """
%%hey

以下のFortran版を作って

#include <stdio.h>
#include <stdlib.h>

double int_inv_1_x2(double a, double b, long n) {
  double s = 0.0;
  double dx = (b - a) / (double)n;
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
    double x = a + i * dx;
    s += 1 / (1 + x * x);
  }
  return s * dx;
}

int main(int argc, char ** argv) {
  double a = (argc > 1 ? atof(argv[1]) : 0.0);
  double b = (argc > 2 ? atof(argv[2]) : 1.0);
  long n   = (argc > 3 ? atol(argv[3]) : 1000L * 1000L * 1000L);
  double s = int_inv_1_x2(a, b, n);
  printf("s = %f\n", s);
  return 0;
}

""" """

""" md

## AIチューターの注意点

* `%%hey` で行った質問とその回答は `hist.sqlite` というファイルに記録される。
* 各ノードブック内で会話を完結・継続するために使われる（各ノードブックがひとつのAIとの対話セッションになる）。
* 課題の評価には使ないが、不正な使い方をしていないかどうかの確認につかうことはあり得る。
* また、今後の授業改善に役立てるため、および今後そのような研究を行うために、このデータを使うことの許諾をいただきたい（任意）。
* 詳細は[ホームページ](https://taura.github.io/computational-science/)を参照

"""

""" md

## `%%bash_` および `%%writefile_`

* `%%bash` および `%%writefile` は Jupyter notebook の元々の機能だが, 本環境では `%%bash_` および `%%writefile_` を使う
* 両者の違いは、後者はAIとの対話とともにそれらの実行が記録されること

"""

""" md

# 本環境の拡張機能2: ジョブ投入を簡便に行うための設定

* Jupyter notebookからジョブ投入を簡便に行うための設定が以下
* 以下は各notebookごとに行う
* 同じnotebookであってもカーネルをログアウトや再スタートしたらやり直す必要がある

"""

""" code """
import wisteria_submit
""" """

""" md

`%%bash` セルと似た使い方で, `%%bash_submit` をセルのはじめに書くとそのセル内のコマンドを計算ノードに投入する

* 使用例

"""

""" code """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L elapse=0:01:00
#PJM -L gpu=1
#PJM --omp thread=9
#PJM -g gt69
#PJM -j
#PJM -o 0output.txt

hostname
pwd
whoami
uptime
""" """

""" md

* なお実は上記で設定されている, `#PJM` オプションは本演習用の典型的な使用例と思われるためデフォルトで設定されている
* 必要に応じて変更したいもの (elapse) を上書きして実行
* %%bash_submit を %%bash_ に変更すればログインノードでの実行になるため, ログインノード実行とジョブ投入での実行を簡単に行き来できる
* `#PJM` で始まる行はログインノードでの実行 (`%%bash`) ではコメントの扱いになるためあっても問題はない

"""

