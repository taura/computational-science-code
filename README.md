# computational-science-code

## 共通設定

1. 学生にもアクセス可能な共有ディレクトリ (`/work/gt69/share/`) の下に venv を導入
1. その環境にAI Tutor HeyとWisteria ジョブ投入ツールを導入

### venv 導入

```
ssh wisteria
python3 -m venv /work/gt69/share/taura/venv/wisteria
. /work/gt69/share/taura/venv/wisteria/bin/activate
pip install --upgrade pip
```

### AI Tutor Hey

```
ssh wisteria
. /work/gt69/share/taura/venv/wisteria/bin/activate
pip install --upgrade git+ssh://git@github.com/UTokyo-IPP/ai-tutor-hey
```

### Wisteria ジョブ投入ツール

```
ssh wisteria
. /work/gt69/share/taura/venv/wisteria/bin/activate
pip install --upgrade git+ssh://git@github.com/taura/wisteria_submit.git
```

## 各学生

```
ssh wisteria
. /work/gt69/share/taura/venv/wisteria/bin/activate
python -m ipykernel install --user --name "wisteria" --display-name "Python 3.14 (wisteria)"
```

下2行を行うスクリプトを `/work/gt69/share/taura/setup_kernel.sh` においたのでこれを実行するのでもOK

```
ssh wisteria
/work/gt69/share/taura/setup_kernel.sh
```

これをやってから https://wisteria08.cc.u-tokyo.ac.jp:8000/jupyterhub/ にアクセスすると `Python 3.14 (wisteria)` というアイコンが現れるのでこれを使う



## リポジトリ構成と教材の更新フロー

このリポジトリ (`computational-science-code`) は教材の **作業用 (非公開)**。
学生にはこの中の `01jupyter/notebooks/source` だけを **別の git リポジトリ**として配布する。

### 構成
- `01jupyter/nb/source/<NN_topic>/` … 教材のソース。`.py` が本文、`include/` に C++/Fortran、
  `problems/<NN>_*/` に練習問題 (**答え付き**。答えは `BEGIN ANSWER` / `END ANSWER` で囲む)。**学生には渡さない。**
- `01jupyter/templates/` … 問題ノートブックのテンプレート (`simple.py`=実行型, `asm.py`=アセンブリ確認型, `app.py`=応用問題用)。
- `01jupyter/Makefile`, `mk_nb.py`, `format_string.py`, `strip_answer.py` … 変換ツール。
- `01jupyter/notebooks/source/` … `make` が生成する **学生向け成果物** (答えは `strip_answer.py` で `TODO` に置換済み)。
  **これ自体が独立した git リポジトリ**で、本リポジトリでは `.gitignore` 済み (追跡しない)。

### 教材を編集して配布するまで
1. `01jupyter/nb/source/...` を編集 (答え入りソース)。
2. `cd 01jupyter && make` で `notebooks/source` を再生成 (答えは自動的に除去される)。
3. 学生リポジトリへ push:
   ```
   cd 01jupyter/notebooks/source
   git add -A && git commit -m "..."
   git push
   ```
4. 本リポジトリへは、ソース側 (`nb/source`, `templates`, `Makefile` 等) の変更をコミットする
   (`notebooks/source` は ignore されるので入らない)。

### 学生リポジトリの初期設定 (最初の一度だけ)
```
cd 01jupyter/notebooks/source
git remote add origin <学生用リポジトリのURL>
git push -u origin main
```
push 後、`01jupyter/notebooks/source/README.md` の `git clone ...` のURLを実際の clone URL に書き換える。

### 注意
- 答え入りファイル (`nb/source` 以下) は学生リポジトリには **含まれない** (別リポジトリかつ生成時に除去)。
  `make` 後に `grep -rl "BEGIN ANSWER" 01jupyter/notebooks/source` が空であることで答えの非混入を確認できる。
- 新しいトピックや問題の作り方は `01jupyter/nb/source/CLAUDE.md` を参照。
