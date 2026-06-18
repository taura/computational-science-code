#!/usr/bin/env python3
"""
解答ファイルから解答部分を取り除き, 学生向けの穴あきコードを生成する.

解答ファイル中で, 解答にあたる行を行コメント形式のマーカーで囲んでおく:

  C/C++:
    // BEGIN ANSWER: <TODO として表示するヒント文>
    double m = (a + b) / 2.0;
    // END ANSWER

  Fortran:
    ! BEGIN ANSWER: <TODO として表示するヒント文>
    m = (a + b) / 2.0d0
    ! END ANSWER

マーカーはコメントなので, 解答ファイルはそのままコンパイルできる.
このスクリプトは BEGIN ANSWER 〜 END ANSWER の範囲を,

    <BEGIN 行のインデント><コメント記号> TODO: <ヒント文>

の 1 行に置き換えて標準出力に書き出す. ヒント文 (": " 以降) が無ければ
"TODO: ここを実装せよ" を用いる.

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
        # 解答ブロックの開始. END ANSWER まで読み飛ばし TODO に置き換える.
        hint = m.group("hint") or "ここを実装せよ"
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
