#!/bin/bash
#------ pjsub option --------#
#PJM -L rscgrp=lecture7-o
#xxxxPJM -L rscgrp=lecture-o
#PJM -L node=1
#xxxxPJM -L gpu=1
#PJM --mpi proc=1
#PJM --omp thread=1
#PJM -L elapse=0:01:00
#PJM -g gt47
#PJM -j
#PJM -o 0done.txt
done_txt=0done.txt
log_txt=0log.txt

# 使い方 :
# (1) batch_main の中に計算ノードで実行したいコマンドを書く
# (2) 必要に応じて 上記 elapse=0:01:00 (制限時間1分)
# (3) 必要に応じて rscgrp=lecture7-o (授業時間中), rscgrp=lecture-o (授業時間外) をセット
# (4) PJM gpu=1 (GPUを使う場合) を指定
# (5) (/work/gt47/ユーザ名 の下のどこかで)
#     ./submit.sh
#
# 普通に pjsub を使うのに比べて便利なところ
# * 実行が何時始まっていつ終わったかがわかりやすい
# * 毎回違う名前の出力ファイルを作らない
# * 標準出力と標準エラーを同じファイルに書いてくれる

batch_main() {
    # ここに, 計算ノードで実行したいコマンドを書く
    echo "whoami: $(whoami)"
    echo "pwd: $(pwd)"
    echo "hostname: $(hostname)"
}

# ここから下は書き換える必要なし
wait_finish() {
    pjstat ${jobid}
    while pjstat ${jobid} | grep ${jobid} | grep QUEUED > /dev/null ; do
        echo -n .
        sleep 1
    done
    echo "started"
    pjstat ${jobid}
    while pjstat ${jobid} | grep ${jobid} > /dev/null ; do
        echo -n .
        sleep 1
    done
    echo 
}

if [ "$PJM_JOBNAME" = "" ] ; then
    rm -f ${done_txt} ${log_txt}
    if [ "$1" = "" ] ; then
        jobid=$(pjsub $0 | grep -o -E 'pjsub Job [0-9]+ submitted' | cut -d ' ' -f 3)
    else
        jobid=$(pjsub -x CMDLINE="$*" $0 | grep -o -E 'pjsub Job [0-9]+ submitted' | cut -d ' ' -f 3)
    fi
    if [ "$jobid" != "" ]; then
        wait_finish
    fi
    cat ${log_txt}
else
    batch_main > ${log_txt} 2>&1
fi
# pjsub --interact -L rscgrp=regular-cache -L node=1 -L elapse=0:01:00 -g gc64
