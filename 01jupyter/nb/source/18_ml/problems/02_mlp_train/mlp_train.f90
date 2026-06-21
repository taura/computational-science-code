! 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
! ネットワーク: 入力 784 (28x28画像) -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
! forward -> softmax クロスエントロピー損失 -> backprop -> ミニバッチ勾配降下 を繰り返す。
! 並列化対象は「ミニバッチ内の全サンプルにわたる勾配の和」(配列 reduction)。
! 配列はすべてネイティブの多次元配列 (Fortran は配列 reduction も配列名のまま書ける)。
! 1サンプルの forward+backprop は forward_backward に, 更新は sgd_update にまとめてある。
! 入出力はNumPy標準の .npy 形式 (read_npy / write_npy を用意)。

module mlp
contains
  ! --- .npy を読み, 中身を「保存順 (C順) のまま」flat な real(8) 配列 a(1:n) に入れる ---
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

  ! --- flat な real(8) 配列 a (C順) を float64 の .npy として書き出す (s1=0 なら1次元) ---
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

  ! --- 状態を持たない乱数 (初期値生成用): (seed,k) から [0,1) ---
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

  ! --- He 初期化 (W(n_in, n_out), バイアスは別途 0 にする) ---
  subroutine init_he(W, salt)
    real(8), intent(out) :: W(:,:)
    integer(8), intent(in) :: salt
    integer :: nin, nout, k, j
    real(8) :: scale
    nin = size(W,1); nout = size(W,2); scale = sqrt(2.0d0/nin) * 2.0d0
    do k = 1, nout
       do j = 1, nin
          W(j,k) = (draw_rand01(int((k-1)*nin+(j-1),8), salt) - 0.5d0) * scale
       end do
    end do
  end subroutine init_he

  ! --- 行列ベクトル積 + バイアス: y = W x + b  (W(n_in, n_out), x(n_in), y(n_out)) ---
  subroutine matvec(W, b, x, y)
    real(8), intent(in)  :: W(:,:), b(:), x(:)
    real(8), intent(out) :: y(:)
    integer :: k, j
    do k = 1, size(W,2)
       y(k) = b(k)
       do j = 1, size(W,1)
          y(k) = y(k) + W(j,k) * x(j)
       end do
    end do
  end subroutine matvec

  ! --- 1サンプルの forward + backprop。勾配 g... に加算し, 損失と正否を返す ---
  subroutine forward_backward(W1, b1, W2, b2, x, label, gW1, gb1, gW2, gb2, sloss, scorr)
    real(8), intent(in)    :: W1(:,:), b1(:), W2(:,:), b2(:), x(:)
    integer, intent(in)    :: label
    real(8), intent(inout) :: gW1(:,:), gb1(:), gW2(:,:), gb2(:)
    real(8), intent(out)   :: sloss
    integer, intent(out)   :: scorr
    integer :: HID, IN, OUT, k, j, c
    real(8) :: h(size(W1,2)), o(size(W2,2)), dout(size(W2,2)), dh
    HID = size(W1,2); IN = size(W1,1); OUT = size(W2,2)
    call matvec(W1, b1, x, h); h = max(0.0d0, h)    ! h = ReLU(W1 x + b1)
    call matvec(W2, b2, h, o)                        ! o = W2 h + b2
    o = exp(o - maxval(o)); o = o / sum(o)           ! p = softmax(o)
    sloss = -log(o(label+1) + 1.0d-12)               ! ラベル 0..9, 添字 1..OUT
    scorr = merge(1, 0, maxloc(o,1)-1 == label)
    dout = o; dout(label+1) = dout(label+1) - 1.0d0  ! do = p - onehot(label)
    do c = 1, OUT                                    ! gW2 += do h^T, gb2 += do
       gb2(c) = gb2(c) + dout(c)
       do k = 1, HID; gW2(k,c) = gW2(k,c) + dout(c)*h(k); end do
    end do
    do k = 1, HID                                    ! dh = (W2^T do)・[h>0]
       if (h(k) <= 0.0d0) cycle
       dh = 0.0d0
       do c = 1, OUT; dh = dh + W2(k,c)*dout(c); end do
       gb1(k) = gb1(k) + dh                          ! gW1 += dh x^T, gb1 += dh
       do j = 1, IN; gW1(j,k) = gW1(j,k) + dh*x(j); end do
    end do
  end subroutine forward_backward

  ! --- 勾配降下の1ステップ: W -= sc * gW,  b -= sc * gb ---
  subroutine sgd_update(W, b, gW, gb, sc)
    real(8), intent(inout) :: W(:,:), b(:)
    real(8), intent(in)    :: gW(:,:), gb(:), sc
    W = W - sc*gW
    b = b - sc*gb
  end subroutine sgd_update
end module mlp

program mlp_train
  use mlp
  use omp_lib
  implicit none
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  character(len=32) :: arg
  integer :: E, BS, ep, s0, s1, nd, scorr
  integer(8) :: N, i, b0, b1n, m
  real(8) :: lr, loss, sc, sloss, t0, elapsed
  integer(8) :: correct
  real(8), allocatable :: X(:,:), W1(:,:), W2(:,:), b1(:), b2(:)
  real(8), allocatable :: gW1(:,:), gW2(:,:), gb1(:), gb2(:), flat(:)
  integer, allocatable :: y(:)

  E = 20; lr = 0.1d0; BS = 100
  if (command_argument_count() >= 1) then; call get_command_argument(1, arg); read(arg,*) E;  end if
  if (command_argument_count() >= 2) then; call get_command_argument(2, arg); read(arg,*) lr; end if
  if (command_argument_count() >= 3) then; call get_command_argument(3, arg); read(arg,*) BS; end if

  ! 訓練データの読み込み (画素 0..255 -> 0..1), X(:,i) が画像 i
  call read_npy("data/x_train.npy", flat, s0, s1, nd); N = s0
  X = reshape(flat, [IN, int(N)]) / 255.0d0
  call read_npy("data/y_train.npy", flat, s0, s1, nd)
  allocate(y(N)); y = nint(flat(1:N))

  ! パラメータ初期化 (He 初期化, バイアスは 0)
  allocate(W1(IN,HID), W2(HID,OUT), b1(HID), b2(OUT))
  allocate(gW1(IN,HID), gW2(HID,OUT), gb1(HID), gb2(OUT))
  call init_he(W1, 1_8); call init_he(W2, 2_8)
  b1 = 0.0d0; b2 = 0.0d0

  loss = 0.0d0; correct = 0
  t0 = omp_get_wtime()
  do ep = 0, E - 1
     loss = 0.0d0; correct = 0
     do b0 = 0, N - 1, BS
        b1n = min(b0 + BS, N)               ! バッチ [b0, b1n)
        m = b1n - b0
        gW1 = 0.0d0; gW2 = 0.0d0; gb1 = 0.0d0; gb2 = 0.0d0

        ! バッチ内の各サンプルの勾配寄与を総和する。各サンプルは独立。
        ! 損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。
        ! BEGIN ANSWER: バッチのループを配列 reduction で並列化せよ: !$omp parallel do private(...) reduction(+:loss,correct,gb2,gW2,gb1,gW1)。
        !$omp parallel do private(i,sloss,scorr) &
        !$omp   reduction(+:loss,correct,gb2,gW2,gb1,gW1)
        ! END ANSWER
        do i = b0 + 1, b1n
           call forward_backward(W1, b1, W2, b2, X(:,i), y(i), &
                                 gW1, gb1, gW2, gb2, sloss, scorr)
           loss = loss + sloss; correct = correct + scorr
        end do
        ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
        !$omp end parallel do
        ! END ANSWER

        sc = lr / real(m, 8)                 ! バッチ内勾配を平均して降下
        call sgd_update(W1, b1, gW1, gb1, sc)
        call sgd_update(W2, b2, gW2, gb2, sc)
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

  ! 学習済みの重みを .npy で書き出す (列優先 W1(IN,HID) の線形並び = C順 (HID,IN))
  call write_npy("data/W1.npy", reshape(W1, [IN*HID]), HID, IN)
  call write_npy("data/b1.npy", b1, HID, 0)
  call write_npy("data/W2.npy", reshape(W2, [HID*OUT]), OUT, HID)
  call write_npy("data/b2.npy", b2, OUT, 0)
  print "(a)", "重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました"
end program mlp_train
