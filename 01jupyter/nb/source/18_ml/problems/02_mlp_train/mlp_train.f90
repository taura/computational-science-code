! 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
! ネットワーク: 入力 784 -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
!
! ミニバッチ (m 枚) を「行列」としてまとめて流す。各ステップはバッチ中の全サンプルを
! 一度に処理する行列演算で, forward / backward は下のプリミティブを呼ぶだけ:
!   forward:  H = ReLU(W1 X + b1) = dense_relu,  P = softmax(W2 H + b2) = dense_softmax
!   backward: dO = P - onehot(y)  = out_grad
!             gW2,gb2 = grad_weight(H, dO),  dH = back_relu(dO, W2, H),  gW1,gb1 = grad_weight(X, dH)
!   更新:     W -= (lr/m) gW
! 勾配は行列積で求まり, サンプル(バッチ)方向の和は行列積の内側の縮約になる。よって
! 並列化は各プリミティブの独立な出力方向ループの parallel do だけでよく, 勾配への配列
! reduction は不要 (損失・正解数のスカラ集計だけ reduction を使う)。
! パラメータ・バッチ・中間行列・勾配を固定サイズ配列で派生型 net_t にまとめる。
! net_t は allocatable 成分を持たないので GPU 発展で map(to: net) がそのまま使える。

module mlp
  implicit none
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  integer, parameter :: MAX_BATCH = 1000        ! ミニバッチの最大サイズ
  type :: net_t
     real(8) :: W1(IN,HID), b1(HID), W2(HID,OUT), b2(OUT)        ! パラメータ
     real(8) :: X(IN,MAX_BATCH), y(MAX_BATCH)                    ! 入力バッチ(列=1枚)とラベル
     real(8) :: H(HID,MAX_BATCH), P(OUT,MAX_BATCH)               ! 中間 (forward)
     real(8) :: dO(OUT,MAX_BATCH), dH(HID,MAX_BATCH)             ! 中間 (backward)
     real(8) :: gW1(IN,HID), gb1(HID), gW2(HID,OUT), gb2(OUT)    ! 勾配
  end type net_t
contains
  ! --- .npy を読み, double として dst に直接書き込む。dst 省略で形だけ。maxrows で頭打ち。 ---
  subroutine read_npy(path, shp, ndim, dst, maxrows)
    character(*), intent(in)  :: path
    integer, intent(in)       :: ndim
    integer, intent(out)      :: shp(ndim)
    real(8), intent(out), optional :: dst(*)
    integer, intent(in),  optional :: maxrows
    integer :: u, hlen, p1, p2, ios, s0, s1, file_ndim
    integer(8) :: n, i, rows
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
    if (.not. present(dst)) then; close(u); return; end if
    rows = s0
    if (present(maxrows)) then; if (maxrows >= 0 .and. s0 > maxrows) rows = maxrows; end if
    n = rows * merge(int(s1,8), 1_8, ndim == 2)

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

  ! ====================== バッチ行列演算 (各ステップが m 枚を一度に処理) ======================
  ! Y = ReLU(W X + b)。W(n_in,n_out), X(n_in,:), Y(n_out,:)。列 i (サンプル) 独立。
  subroutine dense_relu(W, b, X, Y, m)
    real(8), intent(in)  :: W(:,:), b(:), X(:,:)
    real(8), intent(out) :: Y(:,:)
    integer, intent(in)  :: m
    integer :: i, k, j
    real(8) :: s
    ! TODO: 列 i (サンプル) のループを並列化する (各列は独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,k,j,s)
    ! END ANSWER
    do i = 1, m
       do k = 1, size(W,2)
          s = b(k)
          do j = 1, size(W,1); s = s + W(j,k)*X(j,i); end do
          Y(k,i) = max(0.0d0, s)
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine dense_relu

  ! Y = softmax(W X + b) (各列で softmax)。列 i 独立。
  subroutine dense_softmax(W, b, X, Y, m)
    real(8), intent(in)  :: W(:,:), b(:), X(:,:)
    real(8), intent(out) :: Y(:,:)
    integer, intent(in)  :: m
    integer :: i, k, j
    real(8) :: s
    ! TODO: 列 i (サンプル) のループを並列化する (各列は独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,k,j,s)
    ! END ANSWER
    do i = 1, m
       do k = 1, size(W,2)
          s = b(k)
          do j = 1, size(W,1); s = s + W(j,k)*X(j,i); end do
          Y(k,i) = s
       end do
       Y(:,i) = exp(Y(:,i) - maxval(Y(:,i)))
       Y(:,i) = Y(:,i) / sum(Y(:,i))
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine dense_softmax

  ! 出力誤差 dO = P - onehot(y)。列 i 独立。
  subroutine out_grad(P, y, dO, m)
    real(8), intent(in)  :: P(:,:), y(:)
    real(8), intent(out) :: dO(:,:)
    integer, intent(in)  :: m
    integer :: i, c
    ! TODO: 列 i のループを並列化する (各列は独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,c)
    ! END ANSWER
    do i = 1, m
       do c = 1, size(P,1); dO(c,i) = P(c,i) - merge(1.0d0, 0.0d0, c-1 == nint(y(i))); end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine out_grad

  ! 重み勾配: G(a,b) = Σ_i U(a,i) V(b,i),  gb(b) = Σ_i V(b,i)。出力 b (列) ごと独立。
  subroutine grad_weight(U, V, G, gb, m)
    real(8), intent(in)  :: U(:,:), V(:,:)
    real(8), intent(out) :: G(:,:), gb(:)
    integer, intent(in)  :: m
    integer :: a, b, i
    real(8) :: s
    ! TODO: 出力 b のループを並列化する (b ごと独立, バッチ i は内側の和)。
    ! BEGIN ANSWER
    !$omp parallel do private(a,b,i,s)
    ! END ANSWER
    do b = 1, size(G,2)
       s = 0.0d0
       do i = 1, m; s = s + V(b,i); end do
       gb(b) = s
       do a = 1, size(G,1)
          s = 0.0d0
          do i = 1, m; s = s + U(a,i)*V(b,i); end do
          G(a,b) = s
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine grad_weight

  ! 隠れ誤差 dX(k,i) = (Σ_c W(k,c) dY(c,i))・[Href>0]。列 i 独立。
  subroutine back_relu(dY, W, Href, dX, m)
    real(8), intent(in)  :: dY(:,:), W(:,:), Href(:,:)
    real(8), intent(out) :: dX(:,:)
    integer, intent(in)  :: m
    integer :: i, k, c
    real(8) :: s
    ! TODO: 列 i のループを並列化する (各列は独立)。
    ! BEGIN ANSWER
    !$omp parallel do private(i,k,c,s)
    ! END ANSWER
    do i = 1, m
       do k = 1, size(W,1)
          s = 0.0d0
          do c = 1, size(W,2); s = s + W(k,c)*dY(c,i); end do
          dX(k,i) = merge(s, 0.0d0, Href(k,i) > 0.0d0)
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine back_relu

  ! バッチの損失と正解数を集計 (列 i 独立, スカラ reduction)
  subroutine eval(net, m, loss, correct)
    type(net_t), intent(in) :: net
    integer, intent(in) :: m
    real(8), intent(out) :: loss
    integer(8), intent(out) :: correct
    integer :: i
    loss = 0.0d0; correct = 0
    ! TODO: 列 i のループを並列化して損失・正解数を集計する (スカラ reduction)。
    ! BEGIN ANSWER
    !$omp parallel do private(i) reduction(+:loss,correct)
    ! END ANSWER
    do i = 1, m
       loss = loss - log(net%P(nint(net%y(i))+1, i) + 1.0d-12)
       if (maxloc(net%P(:,i),1)-1 == nint(net%y(i))) correct = correct + 1
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine eval

  ! ====================== forward / backward / 更新 (プリミティブを呼ぶだけ) ======================
  subroutine forward(net, m)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    call dense_relu   (net%W1, net%b1, net%X, net%H, m)   ! H = ReLU(W1 X + b1)
    call dense_softmax(net%W2, net%b2, net%H, net%P, m)   ! P = softmax(W2 H + b2)
  end subroutine forward

  subroutine backward(net, m)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    call out_grad   (net%P, net%y, net%dO, m)                 ! dO = P - onehot(y)
    call grad_weight(net%H, net%dO, net%gW2, net%gb2, m)      ! gW2 = Σ_i H dO^T, gb2 = Σ dO
    call back_relu  (net%dO, net%W2, net%H, net%dH, m)        ! dH = (W2^T dO)・[H>0]
    call grad_weight(net%X, net%dH, net%gW1, net%gb1, m)      ! gW1 = Σ_i X dH^T, gb1 = Σ dH
  end subroutine backward

  subroutine sgd_update(net, m, lr)
    type(net_t), intent(inout) :: net
    integer, intent(in) :: m
    real(8), intent(in) :: lr
    real(8) :: sc
    sc = lr / real(m, 8)
    net%W1 = net%W1 - sc*net%gW1; net%b1 = net%b1 - sc*net%gb1
    net%W2 = net%W2 - sc*net%gW2; net%b2 = net%b2 - sc*net%gb2
  end subroutine sgd_update

  ! ====================== データ読み込み ======================
  ! 訓練データ (画像と正解ラベル) を読み込む。画像は 0..1 に正規化し, X,y を確保して返す。
  subroutine load_dataset(xpath, ypath, X, y, N)
    character(*), intent(in) :: xpath, ypath
    real(8), allocatable, intent(out) :: X(:,:), y(:)
    integer(8), intent(out) :: N
    integer :: sh(2)
    call read_npy(xpath, sh, 2)          ! dst 省略で形だけ
    N = sh(1)
    allocate(X(IN, N), y(N))
    call read_npy(xpath, sh, 2, X)       ! 列が1サンプル
    X = X / 255.0d0
    call read_npy(ypath, sh, 1, y)
  end subroutine load_dataset

  ! 全データの b0+1..b0+m の m 枚を net%X, net%y にコピーする
  subroutine load_batch(net, Xall, yall, b0, m)
    type(net_t), intent(inout) :: net
    real(8), intent(in) :: Xall(:,:), yall(:)
    integer(8), intent(in) :: b0
    integer, intent(in) :: m
    integer :: i
    do i = 1, m
       net%X(:, i) = Xall(:, b0 + i)
       net%y(i)    = yall(b0 + i)
    end do
  end subroutine load_batch
end module mlp

program mlp_train
  use mlp
  use omp_lib
  implicit none
  type(net_t), save :: net          ! バッチ・中間行列も含み大きいので静的領域に置く
  character(len=32) :: arg
  integer :: E, BS, ep, m
  integer(8) :: N, b0, bl_correct, correct
  real(8) :: lr, loss, bl_loss, t0, elapsed
  real(8), allocatable :: Xall(:,:), yall(:)

  E = 20; lr = 0.1d0; BS = 100
  if (command_argument_count() >= 1) then; call get_command_argument(1, arg); read(arg,*) E;  end if
  if (command_argument_count() >= 2) then; call get_command_argument(2, arg); read(arg,*) lr; end if
  if (command_argument_count() >= 3) then; call get_command_argument(3, arg); read(arg,*) BS; end if
  if (BS > MAX_BATCH) then; print "(a,i0,a)", "BS は ", MAX_BATCH, " 以下にしてください"; stop 1; end if

  call load_dataset("data/x_train.npy", "data/y_train.npy", Xall, yall, N)

  call init_he(net%W1, 1_8); call init_he(net%W2, 2_8)
  net%b1 = 0.0d0; net%b2 = 0.0d0

  loss = 0.0d0; correct = 0
  t0 = omp_get_wtime()
  do ep = 0, E - 1
     loss = 0.0d0; correct = 0
     do b0 = 0, N - 1, BS
        m = int(min(int(BS,8), N - b0))
        call load_batch(net, Xall, yall, b0, m)
        call forward(net, m)
        call eval(net, m, bl_loss, bl_correct)
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

  call write_npy("data/W1.npy", reshape(net%W1, [IN*HID]), HID, IN)
  call write_npy("data/b1.npy", net%b1, HID, 0)
  call write_npy("data/W2.npy", reshape(net%W2, [HID*OUT]), OUT, HID)
  call write_npy("data/b2.npy", net%b2, OUT, 0)
  print "(a)", "重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました"
end program mlp_train
