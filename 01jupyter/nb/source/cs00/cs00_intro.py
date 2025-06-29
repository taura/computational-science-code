""" md
#* 高性能プログラミングと性能測定(0) --- Jupyter演習環境
"""

""" md w

名前と学生証番号を書け. Enter your name and student ID.

 * 名前 Name:
 * 学生証番号 Student ID:

"""

""" md

# このnotebookの使い方 / How this notebook works

* スーパーコンピュータ(Wisteria)で, SSH + コマンドの代わりに, プログラミングを学習する環境
* 本演習では Wisteria Aquarius (Intel CPU + NVIDIA GPU) を用いる
* Jupyterのセルを実行するだけで例題プログラムを保存, コンパイル, 実行できる
* GPUを用いず, 一瞬で終了するコマンドであればログインノードで直接実行
* それ以外の場合はセルをほんの少し修正するとそれをバッチキューに入れて実行できる

* 入口: https://wisteria08.cc.u-tokyo.ac.jp:8000/jupyterhub/ 
* オリジナルドキュメント: [Wisteria利用支援ポータル](https://wisteria-www.cc.u-tokyo.ac.jp/cgi-bin/hpcportal.ja/index.cgi) にログイン -> ドキュメント閲覧 -> Wisteria/BDEC-01 システム利用手引書 -> 2.7節 JupyterHub

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

# Wisteria上の演習で一度だけやっておくべき作業

"""

""" code """
%%bash
if ! [ -e ~/.notebook/lustre ]; then
  ln -s /work/gt47/$USER ~/.notebook/lustre
fi
""" """

""" md

* 解説
  * Wisteria 上では各ユーザの Jupyter 関連のファイルは `~/.notebook` (ユーザのホームディレクトリ直下の `.notebook`) というフォルダに書かれる
  * ls コマンドは `.` で始まるファイルをデフォルトでは表示しないため見つけにくい
  * `ls -a` とすれば見られるので確認されたい
  * Jupyter のページは `~/.notebook` の下しか表示しない (`~/.notebook` が `/` と表示され, その外は表示できない) 
  * 一方, Wisteria の計算ノードはホームディレクトリを参照できず, Lustreディレクトリ (`/work/グループ名/ユーザ名`) を参照するのが基本
  * したがってこの状態では, Jupyter は `~/.notebook` の下 (したがってホームディレクトリの下) しか表示できず, 計算ノードは Lustre の下しか見えないという不都合な状態になる (例えば Jupyter から書き出したプログラムやデータを計算ノードから実行・参照できない)
  * その回避策として, `~/.notebook` から Lustre ディレクトリへのシンボリックリンクを作っておくのが上記のコマンド

"""

""" md
# 演習用コードのチェックアウト
"""

""" code """
%%bash
cd ~/.notebook/lustre
git clone https://github.com/taura/computational-science-code.git
""" """

""" md
として, ページ左のペインから,
* lustre   # 注: 上記で作ったシンボリックリンク
  * computational-science-code
    * 01jupyter
      * notebooks
        * source
          * cs[00-05]
フォルダ以下にある notebook を開いて見よ          
"""

""" md

# ジョブ投入を簡便に行うための設定

* Jupyter notebookからジョブ投入を簡便に行うための設定が以下
* 以下は各notebookごとに行う
* 同じnotebookであってもカーネルをログアウトや再スタートしたらやり直す必要がある

"""

""" code """
import sys
submit_path = "/work/gt47/share/taura/computational-science-code/00submit"
if submit_path not in sys.path:
    sys.path.append(submit_path)
import submit
""" """

""" md

`%%bash` セルと似た使い方で, `%%bash_submit` をセルのはじめに書くとそのセル内のコマンドを計算ノードに投入する

* 使用例

"""

""" code """
%%bash_submit
#PJM -L rscgrp=lecture-a
#PJM -L gpu=1
#PJM --omp thread=9
#PJM --mpi proc=1
#PJM -L elapse=0:01:00
#PJM -g gt47
#PJM -j
#PJM -o 0output.txt

hostname
pwd
whoami
uptime
""" """

""" md

* なお実は上記で設定されている, `#PJM` オプションは本演習用の典型的な使用例と思われるためデフォルトで設定されており, したがって以下でも上記と同じ動作をする
"""

""" code """
%%bash_submit

hostname
pwd
who
uptime
""" """

""" md

* 必要に応じて上書きしたいものだけを設定すればよい
* 例
"""

""" code """
%%bash_submit
#PJM -L rscgrp=lecture7-a
#PJM -L elapse=0:10:00

hostname
pwd
who
uptime
""" """

""" md

* 以下のように少しだけ変更すればログインノードでの実行になるため, ログインノード実行とジョブ投入での実行を簡単に行き来できる
* `#PJM` で始まる行はログインノードでの実行 (`%%bash`) ではコメントの扱いになるためあっても問題はない

"""

""" code """
%%bash
#_submit
#PJM -L rscgrp=lecture7-a
#PJM -L elapse=0:10:00

hostname
pwd
who
uptime
""" """

