# MNIST 学習済み重みと テスト画像 (data/) を生成するスクリプト (著者用, 学生には配布されない)。
# 実行には MNIST (~/.keras/datasets/mnist.npz 等) が必要。01jupyter ディレクトリで python3 で実行。
import numpy as np
d = np.load('/home/tau/.keras/datasets/mnist.npz')
Xtr = d['x_train'].reshape(-1,784).astype(np.float64)/255.0
ytr = d['y_train'].astype(int)
Xte = d['x_test'].reshape(-1,784).astype(np.float64)/255.0
yte = d['y_test'].astype(int)
IN,HID,OUT = 784,128,10
rng = np.random.default_rng(0)
W1 = rng.standard_normal((HID,IN))*np.sqrt(2.0/IN); b1=np.zeros(HID)
W2 = rng.standard_normal((OUT,HID))*np.sqrt(2.0/HID); b2=np.zeros(OUT)
def fwd(X):
    h = np.maximum(0, X@W1.T + b1)         # ReLU
    o = h@W2.T + b2
    return h,o
def acc(X,y):
    _,o=fwd(X); return (o.argmax(1)==y).mean()
lr=0.1; bs=100; ep=20; n=len(Xtr)
for e in range(ep):
    idx=rng.permutation(n)
    for s in range(0,n,bs):
        b=idx[s:s+bs]; X=Xtr[b]; y=ytr[b]; m=len(b)
        h,o=fwd(X)
        o-=o.max(1,keepdims=True); p=np.exp(o); p/=p.sum(1,keepdims=True)
        g=p; g[np.arange(m),y]-=1; g/=m            # dL/do
        gW2=g.T@h; gb2=g.sum(0)
        gh=(g@W2)*(h>0)
        gW1=gh.T@X; gb1=gh.sum(0)
        W2-=lr*gW2; b2-=lr*gb2; W1-=lr*gW1; b1-=lr*gb1
    if (e+1)%5==0: print(f"epoch {e+1}: test acc = {acc(Xte,yte):.4f}")
full=acc(Xte,yte); print("full test acc:", full)
# 書き出し
NT=1000
out='nb/source/18_ml/problems/00_mnist_infer/data'
with open(out+'/mnist_weights.txt','w') as f:
    f.write(f"{IN} {HID} {OUT}\n")
    np.savetxt(f, W1.reshape(-1)[None], fmt='%.6e'); 
    np.savetxt(f, b1[None], fmt='%.6e')
    np.savetxt(f, W2.reshape(-1)[None], fmt='%.6e')
    np.savetxt(f, b2[None], fmt='%.6e')
# テスト画像 (先頭NT枚) を 0..255 整数 + ラベル
pix=(d['x_test'][:NT].reshape(NT,784)).astype(int)
with open(out+'/mnist_test.txt','w') as f:
    f.write(f"{NT} {IN}\n")
    for i in range(NT):
        f.write(' '.join(map(str,pix[i])) + f' {int(yte[i])}\n')
sub=acc(Xte[:NT],yte[:NT]); print(f"subset({NT}) test acc:", round(sub,4))
