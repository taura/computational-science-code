! 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
! ネットワーク: 入力 784 (28x28画像) -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
! forward -> softmax クロスエントロピー損失 -> backprop -> ミニバッチ勾配降下 を繰り返す。
! 並列化対象は「ミニバッチ内の全サンプルにわたる勾配の和」(配列 reduction)。
! 入出力はNumPy標準の .npy 形式:
!   読み: data/x_train.npy (uint8 [N,784]), data/y_train.npy (int32 [N])
!   書き: data/W1.npy, b1.npy, W2.npy, b2.npy (float64) -> 00_mnist_infer が読む
! read_npy / write_npy は下のモジュールに用意してある (I/O は主眼ではない)。

module npy_io
contains
  ! .npy を読み, 中身を「保存順 (C順) のまま」flat な real(8) 配列 a(1:n) に入れる。
  subroutine read_npy(path, a, s0, s1, ndim)
    character(*), intent(in) :: path
    real(8), allocatable, intent(out) :: a(:)
    integer, intent(out) :: s0, s1, ndim
    integer :: u, hlen, p1, p2, ios
    integer(8) :: n, i
    character(len=10) :: magic
    character(len=:), allocatable :: hdr, sub
    character(len=8) :: descr
    integer(1), allocatable :: t1(:)
    integer(4), allocatable :: t4(:)
    integer(8), allocatable :: t8(:)
    real(4),    allocatable :: r4(:)

    open(newunit=u, file=path, access='stream', form='unformatted', &
         status='old', action='read')
    read(u) magic
    if (magic(1:6) /= char(147)//'NUMPY') stop '.npy ではありません'
    hlen = ichar(magic(9:9)) + 256*ichar(magic(10:10))
    allocate(character(len=hlen) :: hdr)
    read(u) hdr
    if      (index(hdr,"'<f8'") > 0) then; descr = '<f8'
    else if (index(hdr,"'<f4'") > 0) then; descr = '<f4'
    else if (index(hdr,"'|u1'") > 0) then; descr = '|u1'
    else if (index(hdr,"'<i4'") > 0) then; descr = '<i4'
    else if (index(hdr,"'<i8'") > 0) then; descr = '<i8'
    else; stop '未対応の dtype'; end if
    p1 = index(hdr, '('); p2 = index(hdr, ')')
    sub = hdr(p1+1:p2-1)
    do i = 1, len(sub); if (sub(i:i) == ',') sub(i:i) = ' '; end do
    read(sub, *, iostat=ios) s0, s1
    if (ios /= 0) then; ndim = 1; s1 = 1; read(sub, *) s0
    else;               ndim = 2; end if
    n = int(s0,8) * merge(int(s1,8), 1_8, ndim == 2)

    allocate(a(n))
    select case (trim(descr))
    case ('<f8'); read(u) a
    case ('<f4'); allocate(r4(n)); read(u) r4; a = real(r4,8)
    case ('|u1'); allocate(t1(n)); read(u) t1
                  do i = 1, n; a(i) = real(merge(int(t1(i))+256, int(t1(i)), t1(i) < 0), 8); end do
    case ('<i4'); allocate(t4(n)); read(u) t4; a = real(t4,8)
    case ('<i8'); allocate(t8(n)); read(u) t8; a = real(t8,8)
    end select
    close(u)
  end subroutine read_npy

  ! flat な real(8) 配列 a (C順) を float64 の .npy として書き出す。s1=0 なら1次元。
  subroutine write_npy(path, a, s0, s1)
    character(*), intent(in) :: path
    real(8), intent(in) :: a(:)
    integer, intent(in) :: s0, s1
    integer :: u, base, pad, hlen, i
    character(len=64) :: shp
    character(len=:), allocatable :: dict

    if (s1 > 0) then; write(shp,'(a,i0,a,i0,a)') '(', s0, ', ', s1, ')'
    else;             write(shp,'(a,i0,a)')      '(', s0, ',)'; end if
    dict = "{'descr': '<f8', 'fortran_order': False, 'shape': " // trim(shp) // ", }"
    base = 10 + len(dict) + 1                  ! +1 は末尾の改行
    pad  = mod(64 - mod(base, 64), 64)         ! 全体を64の倍数に揃える
    hlen = len(dict) + 1 + pad

    open(newunit=u, file=path, access='stream', form='unformatted', &
         status='replace', action='write')
    write(u) char(147)//'NUMPY'//char(1)//char(0)
    write(u) char(mod(hlen,256)), char(hlen/256)
    write(u) dict
    do i = 1, pad; write(u) ' '; end do
    write(u) char(10)
    write(u) a
    close(u)
  end subroutine write_npy

  ! 状態を持たない乱数 (初期値生成用): (seed,k) から [0,1)。
  function draw_rand01(seed, k) result(u)
    integer(8), intent(in) :: seed, k
    real(8) :: u
    integer(8), parameter :: M = 2147483647_8
    integer(8) :: x
    x = modulo(modulo(seed, M) * 2654435761_8 + modulo(k, M) + 1_8, M)
    x = modulo(ieor(x, ishft(x, -16)) * 1812433253_8, M)
    x = modulo(ieor(x, ishft(x, -13)) * 1664525_8,    M)
    x = modulo(ieor(x, ishft(x, -16)), M)
    u = real(x, 8) / real(M, 8)
  end function draw_rand01
end module npy_io

program mlp_train
  use npy_io
  use omp_lib
  implicit none
  ! flat な 0始まり配列で C 版と同じ並びにする (W1(k*IN+j)=W1[k][j] 等)
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  character(len=32) :: arg
  integer :: E, ep, c, kk, s0, s1, nd, best
  integer(8) :: N, i, b0, b1n, m
  real(8) :: lr, loss, z, omax, ssum, sc, bestv, t0, elapsed, dh, sq1, sq2
  integer(8) :: correct
  real(8), allocatable :: X(:), W1(:), b1(:), W2(:), b2(:)
  real(8), allocatable :: gW1(:), gb1(:), gW2(:), gb2(:), flat(:)
  real(8), allocatable :: h(:), o(:), dout(:)
  integer, allocatable :: y(:)

  E = 20; lr = 0.1d0
  if (command_argument_count() >= 1) then; call get_command_argument(1, arg); read(arg,*) E;  end if
  if (command_argument_count() >= 2) then; call get_command_argument(2, arg); read(arg,*) lr; end if
  ! ミニバッチサイズ (既定 100)
  block
    integer :: BS
    BS = 100
    if (command_argument_count() >= 3) then; call get_command_argument(3, arg); read(arg,*) BS; end if

    ! --- 訓練データの読み込み (画素 0..255 -> 0..1) ---
    call read_npy("data/x_train.npy", flat, s0, s1, nd); N = s0
    allocate(X(0:N*IN-1)); X = flat / 255.0d0
    call read_npy("data/y_train.npy", flat, s0, s1, nd)
    allocate(y(0:N-1)); y = nint(flat(1:N))

    ! --- パラメータ初期化 (He 初期化) ---
    allocate(W1(0:HID*IN-1), b1(0:HID-1), W2(0:OUT*HID-1), b2(0:OUT-1))
    allocate(gW1(0:HID*IN-1), gb1(0:HID-1), gW2(0:OUT*HID-1), gb2(0:OUT-1))
    sq1 = sqrt(2.0d0/IN); sq2 = sqrt(2.0d0/HID)
    do i = 0, int(HID,8)*IN-1; W1(i) = (draw_rand01(i,1_8)-0.5d0)*2.0d0*sq1; end do
    b1 = 0.0d0
    do i = 0, int(OUT,8)*HID-1; W2(i) = (draw_rand01(i,2_8)-0.5d0)*2.0d0*sq2; end do
    b2 = 0.0d0

    allocate(h(0:HID-1), o(0:OUT-1), dout(0:OUT-1))
    loss = 0.0d0; correct = 0
    t0 = omp_get_wtime()
    do ep = 0, E - 1
       loss = 0.0d0; correct = 0
       do b0 = 0, N - 1, BS
          b1n = min(b0 + BS, N)               ! バッチ [b0, b1n)
          m = b1n - b0
          gW1 = 0.0d0; gb1 = 0.0d0; gW2 = 0.0d0; gb2 = 0.0d0

          ! バッチ内の全サンプルにわたる forward + backprop。勾配を総和する。
          ! 損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。
          ! BEGIN ANSWER: バッチのループを配列 reduction で並列化せよ: !$omp parallel do private(...) reduction(+:loss,correct,gb2,gW2,gb1,gW1) (h,o,dout は private)。
          !$omp parallel do private(i,kk,c,z,omax,ssum,best,bestv,dh,h,o,dout) &
          !$omp   reduction(+:loss,correct,gb2,gW2,gb1,gW1)
          ! END ANSWER
          do i = b0, b1n - 1
             do kk = 0, HID - 1               ! h = ReLU(W1 x + b1)
                z = b1(kk)
                do c = 0, IN - 1
                   z = z + W1(kk*IN+c) * X(i*IN+c)
                end do
                h(kk) = max(0.0d0, z)
             end do
             omax = -1d300                     ! o = W2 h + b2
             do c = 0, OUT - 1
                z = b2(c)
                do kk = 0, HID - 1
                   z = z + W2(c*HID+kk) * h(kk)
                end do
                o(c) = z; if (z > omax) omax = z
             end do
             ssum = 0.0d0                       ! softmax
             do c = 0, OUT - 1; o(c) = exp(o(c)-omax); ssum = ssum + o(c); end do
             best = 0; bestv = -1.0d0
             do c = 0, OUT - 1
                o(c) = o(c)/ssum
                if (o(c) > bestv) then; bestv = o(c); best = c; end if
             end do
             loss = loss - log(o(y(i)) + 1.0d-12)
             if (best == y(i)) correct = correct + 1
             ! backprop: do = p - onehot(y)
             do c = 0, OUT - 1; dout(c) = o(c) - merge(1.0d0,0.0d0,c==y(i)); end do
             do c = 0, OUT - 1
                gb2(c) = gb2(c) + dout(c)
                do kk = 0, HID - 1
                   gW2(c*HID+kk) = gW2(c*HID+kk) + dout(c)*h(kk)
                end do
             end do
             do kk = 0, HID - 1                 ! dh = (W2^T do)・[h>0]
                if (h(kk) <= 0.0d0) cycle
                dh = 0.0d0
                do c = 0, OUT - 1; dh = dh + W2(c*HID+kk)*dout(c); end do
                gb1(kk) = gb1(kk) + dh
                do c = 0, IN - 1
                   gW1(kk*IN+c) = gW1(kk*IN+c) + dh * X(i*IN+c)
                end do
             end do
          end do
          ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
          !$omp end parallel do
          ! END ANSWER

          ! 更新 (バッチ内勾配を平均して降下)
          sc = lr / real(m, 8)
          W1 = W1 - sc*gW1; b1 = b1 - sc*gb1; W2 = W2 - sc*gW2; b2 = b2 - sc*gb2
       end do
       loss = loss / real(N, 8)
       if (mod(ep,5) == 0 .or. ep == E-1) &
          print "(a,i4,a,f7.4,a,f6.2,a)", "epoch ", ep, ": loss=", loss, &
                ", train acc=", 100.0d0*correct/N, "%"
    end do
    elapsed = omp_get_wtime() - t0

    print "(a,i0,a,i0,a,i0,a,f7.4,a,f6.2,a)", "最終: N=", N, ", HID=", HID, &
         ", epochs=", E, ", loss=", loss, ", train acc=", 100.0d0*correct/N, "%"
    print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
  end block

  ! --- 学習済みの重みを .npy で書き出す (00_mnist_infer が読む) ---
  call write_npy("data/W1.npy", W1, HID, IN)
  call write_npy("data/b1.npy", b1, HID, 0)
  call write_npy("data/W2.npy", W2, OUT, HID)
  call write_npy("data/b2.npy", b2, OUT, 0)
  print "(a)", "重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました"
end program mlp_train
