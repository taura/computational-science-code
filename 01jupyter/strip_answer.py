#!/usr/bin/env python3
"""
解答ファイルから解答部分を取り除き, 学生向けの穴あきコードを生成する.

解答ファイル中で, 解答にあたる行を行コメント形式のマーカーで囲んでおく.
ヒントは別行の通常コメント (// TODO: ... など) で与え, マーカー自身は
学生向けコードから消す (BEGIN/END の行も解答行も出力しない):

  C/C++:
    // TODO: ここを並列化する          ← 通常コメント. そのまま残る
    // BEGIN ANSWER
    #pragma omp parallel for
    // END ANSWER

  Fortran:
    ! TODO: ここを並列化する
    ! BEGIN ANSWER
    !$omp parallel do
    ! END ANSWER

マーカーはコメントなので, 解答ファイルはそのままコンパイルできる.
このスクリプトは BEGIN ANSWER 〜 END ANSWER の範囲 (マーカー行を含む) を
出力から取り除く. ヒントは上のように別の通常コメント行で書けばよい.

後方互換: 古い形式 `BEGIN ANSWER: <ヒント文>` のようにマーカー行にヒントを
書いた場合は, その範囲を

    <BEGIN 行のインデント><コメント記号> TODO: <ヒント文>

の 1 行に置き換える (従来動作).

使い方:
    ./strip_answer.py 解答ファイル > 穴あきファイル
"""
import re
import sys

# 行頭の空白 + コメント記号 (// または !) + "BEGIN ANSWER" [: ヒント]
BEGIN = re.compile(r'^(?P<indent>\s*)(?P<comment>//|!)\s*BEGIN ANSWER\s*:?\s*(?P<hint>.*?)\s*$')
END = re.compile(r'^\s*(?://|!)\s*END ANSWER\b')


def strip(lines):
    out = []
    it = iter(lines)
    for line in it:
        m = BEGIN.match(line)
        if not m:
            out.append(line)
            continue
        # 解答ブロックの開始. END ANSWER まで読み飛ばして取り除く.
        # マーカー行にヒントが書かれていれば (古い形式) TODO 行に置き換える.
        # bare な BEGIN ANSWER (ヒント無し) なら何も出力しない.
        hint = m.group("hint")
        if hint:
            out.append("{indent}{comment} TODO: {hint}\n".format(
                indent=m.group("indent"), comment=m.group("comment"), hint=hint))
        for inner in it:
            if END.match(inner):
                break
        else:
            sys.stderr.write("strip_answer.py: BEGIN ANSWER に対応する "
                             "END ANSWER がありません\n")
            sys.exit(1)
    return out


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("usage: {} FILE\n".format(sys.argv[0]))
        sys.exit(1)
    with open(sys.argv[1]) as fp:
        sys.stdout.write("".join(strip(fp.readlines())))


if __name__ == "__main__":
    main()
