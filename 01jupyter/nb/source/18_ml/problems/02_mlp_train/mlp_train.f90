! 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
! ネットワーク: 入力 784 -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
!
! ミニバッチ (m 枚) を「行列」として一度に流す:
!   forward:  H = ReLU(W1 X + b1),  P = softmax(W2 H + b2)   (列が1サンプル)
!   backward: dO = P - onehot(y),
!             gW2 = dO H^T,  gb2 = Σ_列 dO,
!             dH  = (W2^T dO)・[H>0],  gW1 = dH X^T,  gb1 = Σ_列 dH
!   更新:     W -= (lr/m) gW
! 勾配は行列積で求まり, サンプル(バッチ)方向の和は行列積の内側の縮約になる。よって
! 並列化は各行列積の独立な出力方向の単純な parallel do でよく, 配列 reduction は不要
! (損失・正解数のスカラ集計だけ reduction を使う)。
! パラメータ・バッチ・中間行列・勾配を固定サイズ配列で派生型 net_t にまとめる。
! net_t は allocatable 成分を持たないので GPU 発展で map(to: net) がそのまま使える。

module mlp
  implicit none
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  integer, parameter :: MAX_BATCH = 1000        ! ミニバッチの最大サイズ
  type :: net_t
     real(8) :: W1(IN,HID), b1(HID), W2(HID,OUT), b2(OUT)        ! パラメータ
     real(8) :: X(IN,MAX_BATCH)                                  ! 入力バッチ (列=1サンプル)
     integer :: y(MAX_BATCH)                                     ! ラベルバッチ
     real(8) :: H(HID,MAX_BATCH), P(OUT,MAX_BATCH)               ! 中間 (forward)
     real(8) :: dO(OUT,MAX_BATCH), dH(HID,MAX_BATCH)             ! 中間 (backward)
     real(8) :: gW1(IN,HID), gb1(HID), gW2(HID,OUT), gb2(OUT)    ! 勾配
  end type net_t
contains
  ! --- .npy を読み, 任意の数値型を double として dst に直接書き込む (C順)。
  !     形を shp(1..ndim) に返す。dst を省略すると形だけ取得 (peek)。 ---
  subroutine read_npy(path, shp, ndim, dst)
    character(*), intent(in)  :: path
    integer, intent(in)       :: ndim
    integer, intent(out)      :: shp(ndim)
    real(8), intent(out), optional :: dst(*)
    integer :: u, hlen, p1, p2, ios, s0, s1, file_ndim
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
    if (ios /= 0) then; file_ndim = 1; s1 = 1; read(sub, *) s0
    else;               file_ndim = 2; end if
    if (file_ndim /= ndim) then
       print "(a,a,i0,a,i0,a)", trim(path), ": ", ndim, " 次元を期待しましたが ", file_ndim, " 次元でした"
       stop 1
    end if
    shp(1) = s0
    if (ndim == 2) shp(2) = s1
    if (.not. present(dst)) then; close(u); return; end if   ! 形だけ
    n = int(s0,8) * merge(int(s1,8), 1_8, ndim == 2)

    select case (trim(descr))
    case ('<f8'); read(u) dst(1:n)
    case ('<f4'); allocate(r4(n)); read(u) r4; dst(1:n) = real(r4,8)
    case ('|u1'); allocate(t1(n)); read(u) t1
                  do i = 1, n; dst(i) = real(merge(int(t1(i))+256, int(t1(i)), t1(i) < 0), 8); end do
    case ('<i4'); allocate(t4(n)); read(u) t4; dst(1:n) = real(t4,8)
    case ('<i8'); allocate(t8(n)); read(u) t8; dst(1:n) = real(t8,8)
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
    base = 10 + len(dict) + 1
    pad  = mod(64 - mod(base, 64), 64)
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

  ! --- He 初期化 (W(n_in, n_out)) ---
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

  ! --- 行列ベクトル積 + バイアス: y = W x + b  (W(n_in,n_out), assumed-shape) ---
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

  subroutine softmax_inplace(v)
    real(8), intent(inout) :: v(:)
    v = exp(v - maxval(v)); v = v / sum(v)
  end subroutine softmax_inplace

  ! --- forward: 各サンプル列 i は独立。H(:,i),P(:,i) を埋め, 損失と正解数を集計 ---
  subroutine forward(net, m, loss, correct)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    real(8), intent(out) :: loss
    integer(8), intent(out) :: correct
    integer :: i
    loss = 0.0d0; correct = 0
    ! TODO: 各サンプル列 i の forward を並列化する (列ごと独立, 損失・正解数はスカラ集計)。
    ! BEGIN ANSWER
    !$omp parallel do private(i) reduction(+:loss,correct)
    ! END ANSWER
    do i = 1, m
       call matvec(net%W1, net%b1, net%X(:,i), net%H(:,i)); net%H(:,i) = max(0.0d0, net%H(:,i))
       call matvec(net%W2, net%b2, net%H(:,i), net%P(:,i)); call softmax_inplace(net%P(:,i))
       loss = loss - log(net%P(net%y(i)+1, i) + 1.0d-12)
       if (maxloc(net%P(:,i),1)-1 == net%y(i)) correct = correct + 1
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine forward

  ! --- backward: 勾配を行列積で求める。各行列積の出力方向は独立なので reduction 不要 ---
  subroutine backward(net, m)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    integer :: i, k, c, j
    real(8) :: s

    ! 出力誤差 dO = P - onehot(y) : 列 i ごと独立
    ! TODO: dO を計算するループを並列化する (列 i ごと独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,c)
    ! END ANSWER
    do i = 1, m
       do c = 1, OUT
          net%dO(c,i) = net%P(c,i) - merge(1.0d0, 0.0d0, c == net%y(i)+1)
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER

    ! gW2(k,c) = Σ_i H(k,i) dO(c,i), gb2(c) = Σ_i dO(c,i) : 出力 c ごと独立
    ! TODO: gW2,gb2 を計算するループを並列化する (出力 c ごと独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(c,k,i,s)
    ! END ANSWER
    do c = 1, OUT
       s = 0.0d0
       do i = 1, m; s = s + net%dO(c,i); end do
       net%gb2(c) = s
       do k = 1, HID
          s = 0.0d0
          do i = 1, m; s = s + net%H(k,i) * net%dO(c,i); end do
          net%gW2(k,c) = s
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER

    ! 隠れ誤差 dH(k,i) = (Σ_c W2(k,c) dO(c,i))・[H>0] : 列 i ごと独立
    ! TODO: dH を計算するループを並列化する (列 i ごと独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,k,c,s)
    ! END ANSWER
    do i = 1, m
       do k = 1, HID
          s = 0.0d0
          do c = 1, OUT; s = s + net%W2(k,c) * net%dO(c,i); end do
          net%dH(k,i) = merge(s, 0.0d0, net%H(k,i) > 0.0d0)
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER

    ! gW1(j,k) = Σ_i X(j,i) dH(k,i), gb1(k) = Σ_i dH(k,i) : 隠れ k ごと独立
    ! TODO: gW1,gb1 を計算するループを並列化する (隠れ k ごと独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(k,j,i,s)
    ! END ANSWER
    do k = 1, HID
       s = 0.0d0
       do i = 1, m; s = s + net%dH(k,i); end do
       net%gb1(k) = s
       do j = 1, IN
          s = 0.0d0
          do i = 1, m; s = s + net%X(j,i) * net%dH(k,i); end do
          net%gW1(j,k) = s
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine backward

  ! --- 勾配降下の1ステップ: W -= (lr/m) gW ---
  subroutine sgd_update(net, m, lr)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    real(8), intent(in) :: lr
    real(8) :: sc
    sc = lr / real(m, 8)
    net%W1 = net%W1 - sc*net%gW1; net%b1 = net%b1 - sc*net%gb1
    net%W2 = net%W2 - sc*net%gW2; net%b2 = net%b2 - sc*net%gb2
  end subroutine sgd_update
end module mlp

program mlp_train
  use mlp
  use omp_lib
  implicit none
  type(net_t), save :: net          ! バッチ・中間行列も含み大きいので静的領域に置く
  character(len=32) :: arg
  integer :: E, BS, ep, sh(2), i, m
  integer(8) :: N, b0, bl_correct, correct
  real(8) :: lr, loss, bl_loss, t0, elapsed
  real(8), allocatable :: Xall(:,:), yd(:)
  integer, allocatable :: yall(:)

  E = 20; lr = 0.1d0; BS = 100
  if (command_argument_count() >= 1) then; call get_command_argument(1, arg); read(arg,*) E;  end if
  if (command_argument_count() >= 2) then; call get_command_argument(2, arg); read(arg,*) lr; end if
  if (command_argument_count() >= 3) then; call get_command_argument(3, arg); read(arg,*) BS; end if
  if (BS > MAX_BATCH) then; print "(a,i0,a)", "BS は ", MAX_BATCH, " 以下にしてください"; stop 1; end if

  ! 訓練データ全体をヒープに読む。まず形だけ見て N を得てから確保する。
  call read_npy("data/x_train.npy", sh, 2)          ! dst 省略で形だけ
  N = sh(1)
  allocate(Xall(IN, N), yd(N), yall(N))
  call read_npy("data/x_train.npy", sh, 2, Xall)    ! 列が1サンプル
  Xall = Xall / 255.0d0
  call read_npy("data/y_train.npy", sh, 1, yd)
  yall = nint(yd)

  ! パラメータ初期化 (He 初期化, バイアスは 0)
  call init_he(net%W1, 1_8); call init_he(net%W2, 2_8)
  net%b1 = 0.0d0; net%b2 = 0.0d0

  loss = 0.0d0; correct = 0
  t0 = omp_get_wtime()
  do ep = 0, E - 1
     loss = 0.0d0; correct = 0
     do b0 = 0, N - 1, BS
        m = int(min(int(BS,8), N - b0))
        ! 今のバッチを net%X, net%y にコピー
        do i = 1, m
           net%X(:, i) = Xall(:, b0 + i)
           net%y(i)    = yall(b0 + i)
        end do
        call forward(net, m, bl_loss, bl_correct)
        loss = loss + bl_loss; correct = correct + bl_correct
        call backward(net, m)
        call sgd_update(net, m, lr)
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
  call write_npy("data/W1.npy", reshape(net%W1, [IN*HID]), HID, IN)
  call write_npy("data/b1.npy", net%b1, HID, 0)
  call write_npy("data/W2.npy", reshape(net%W2, [HID*OUT]), OUT, HID)
  call write_npy("data/b2.npy", net%b2, OUT, 0)
  print "(a)", "重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました"
end program mlp_train
