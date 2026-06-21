! 多層パーセプトロン (MLP) を自分で学習させ, 本物の MNIST 手書き数字を分類する。
! ネットワーク: 入力 784 -> 隠れ層 HID=128 (ReLU) -> 出力 10クラス。
! forward -> softmax クロスエントロピー損失 -> backprop -> ミニバッチ勾配降下 を繰り返す。
! 並列化対象は「ミニバッチ内の全サンプルにわたる勾配の和」(配列 reduction)。
! ネットワークの大きさは定数なので, パラメータは固定サイズ配列を持つ派生型 Net にまとめる。
! 勾配は配列 reduction にかけるためネイティブ2次元配列のまま (whole-array reduction)。
! matvec/matTvec は assumed-shape でサイズによらず1つで効く。Net は allocatable 成分を
! 持たないので GPU 発展で map(to: net) がそのまま使える。

module mlp
  implicit none
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  type :: net_t
     real(8) :: W1(IN,HID), b1(HID), W2(HID,OUT), b2(OUT)
  end type net_t
contains
  ! --- .npy を読み, flat な real(8) 配列に入れる。ndim は呼び出し側指定, 違えばエラー ---
  subroutine read_npy(path, a, shp, ndim)
    character(*), intent(in) :: path
    real(8), allocatable, intent(out) :: a(:)
    integer, intent(in)  :: ndim
    integer, intent(out) :: shp(ndim)
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

  ! --- 転置行列ベクトル積: y = W^T d  (W(n_in,n_out), d は n_out, y は n_in) ---
  subroutine matTvec(W, d, y)
    real(8), intent(in)  :: W(:,:), d(:)
    real(8), intent(out) :: y(:)
    integer :: k, j
    y = 0.0d0
    do k = 1, size(W,2)
       do j = 1, size(W,1)
          y(j) = y(j) + W(j,k) * d(k)
       end do
    end do
  end subroutine matTvec

  ! --- 外積の加算 (rank-1 更新): G += a b^T  (G(m,n), a は m, b は n) ---
  subroutine add_outer(G, a, b)
    real(8), intent(inout) :: G(:,:)
    real(8), intent(in)    :: a(:), b(:)
    integer :: i, j
    do j = 1, size(G,2)
       do i = 1, size(G,1)
          G(i,j) = G(i,j) + a(i) * b(j)
       end do
    end do
  end subroutine add_outer

  ! --- 1サンプルの forward + backprop。勾配 g... に加算し, 損失と正否を返す ---
  subroutine forward_backward(net, x, label, gW1, gb1, gW2, gb2, sloss, scorr)
    type(net_t), intent(in)  :: net
    real(8), intent(in)    :: x(:)
    integer, intent(in)    :: label
    real(8), intent(inout) :: gW1(:,:), gb1(:), gW2(:,:), gb2(:)
    real(8), intent(out)   :: sloss
    integer, intent(out)   :: scorr
    real(8) :: h(HID), o(OUT), dout(OUT), dh(HID)
    call matvec(net%W1, net%b1, x, h); h = max(0.0d0, h)   ! h = ReLU(W1 x + b1)
    call matvec(net%W2, net%b2, h, o)                       ! o = W2 h + b2
    o = exp(o - maxval(o)); o = o / sum(o)                  ! p = softmax(o)
    sloss = -log(o(label+1) + 1.0d-12)                      ! ラベル 0..9, 添字 1..OUT
    scorr = merge(1, 0, maxloc(o,1)-1 == label)
    dout = o; dout(label+1) = dout(label+1) - 1.0d0         ! do = p - onehot(label)
    gb2 = gb2 + dout                                        ! gb2 += do
    call add_outer(gW2, h, dout)                            ! gW2 += do h^T (= h ⊗ do)
    call matTvec(net%W2, dout, dh)                          ! dh = W2^T do
    where (h <= 0.0d0) dh = 0.0d0                           ! ReLU の微分 (・[h>0])
    gb1 = gb1 + dh                                          ! gb1 += dh
    call add_outer(gW1, x, dh)                              ! gW1 += dh x^T (= x ⊗ dh)
  end subroutine forward_backward

  ! --- 勾配降下の1ステップ: W -= sc gW,  b -= sc gb ---
  subroutine sgd_update(net, gW1, gb1, gW2, gb2, sc)
    type(net_t), intent(inout) :: net
    real(8), intent(in)      :: gW1(:,:), gb1(:), gW2(:,:), gb2(:), sc
    net%W1 = net%W1 - sc*gW1; net%b1 = net%b1 - sc*gb1
    net%W2 = net%W2 - sc*gW2; net%b2 = net%b2 - sc*gb2
  end subroutine sgd_update
end module mlp

program mlp_train
  use mlp
  use omp_lib
  implicit none
  type(net_t) :: net
  character(len=32) :: arg
  integer :: E, BS, ep, sh1(1), sh2(2), scorr
  integer(8) :: N, i, b0, b1n, m
  real(8) :: lr, loss, sc, sloss, t0, elapsed
  integer(8) :: correct
  real(8), allocatable :: X(:,:), gW1(:,:), gW2(:,:), gb1(:), gb2(:), flat(:)
  integer, allocatable :: y(:)

  E = 20; lr = 0.1d0; BS = 100
  if (command_argument_count() >= 1) then; call get_command_argument(1, arg); read(arg,*) E;  end if
  if (command_argument_count() >= 2) then; call get_command_argument(2, arg); read(arg,*) lr; end if
  if (command_argument_count() >= 3) then; call get_command_argument(3, arg); read(arg,*) BS; end if

  ! 訓練データ (枚数 N は可変) を読み, 画素 0..255 -> 0..1。X(:,i) が画像 i
  call read_npy("data/x_train.npy", flat, sh2, 2); N = sh2(1)
  X = reshape(flat, [IN, int(N)]) / 255.0d0
  call read_npy("data/y_train.npy", flat, sh1, 1)
  allocate(y(N)); y = nint(flat(1:N))

  ! パラメータ初期化 (He 初期化, バイアスは 0)。勾配はネイティブ2次元配列。
  call init_he(net%W1, 1_8); call init_he(net%W2, 2_8)
  net%b1 = 0.0d0; net%b2 = 0.0d0
  allocate(gW1(IN,HID), gW2(HID,OUT), gb1(HID), gb2(OUT))

  loss = 0.0d0; correct = 0
  t0 = omp_get_wtime()
  do ep = 0, E - 1
     loss = 0.0d0; correct = 0
     do b0 = 0, N - 1, BS
        b1n = min(b0 + BS, N); m = b1n - b0
        gW1 = 0.0d0; gW2 = 0.0d0; gb1 = 0.0d0; gb2 = 0.0d0

        ! バッチ内の各サンプルの勾配寄与を総和する。各サンプルは独立。
        ! TODO: このバッチ内のループを並列化する (各サンプルは独立)。
        ! BEGIN ANSWER
        !$omp parallel do private(i,sloss,scorr) &
        !$omp   reduction(+:loss,correct,gb2,gW2,gb1,gW1)
        ! END ANSWER
        do i = b0 + 1, b1n
           call forward_backward(net, X(:,i), y(i), gW1, gb1, gW2, gb2, sloss, scorr)
           loss = loss + sloss; correct = correct + scorr
        end do
        ! BEGIN ANSWER
        !$omp end parallel do
        ! END ANSWER

        sc = lr / real(m, 8)
        call sgd_update(net, gW1, gb1, gW2, gb2, sc)
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

  ! 学習済みの重みを .npy で書き出す (列優先 net%W1(IN,HID) の線形並び = C順 (HID,IN))
  call write_npy("data/W1.npy", reshape(net%W1, [IN*HID]), HID, IN)
  call write_npy("data/b1.npy", net%b1, HID, 0)
  call write_npy("data/W2.npy", reshape(net%W2, [HID*OUT]), OUT, HID)
  call write_npy("data/b2.npy", net%b2, OUT, 0)
  print "(a)", "重みを data/W1.npy, b1.npy, W2.npy, b2.npy に保存しました"
end program mlp_train
