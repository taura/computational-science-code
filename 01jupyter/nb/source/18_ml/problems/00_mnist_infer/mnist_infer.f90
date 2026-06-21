! 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
!   data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
!   data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
!   data/y_test.npy : 正解ラベル (int32 [N], 0..9)
! 推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」(下の matvec / predict)。
! 各画像の推論は独立なので並列化できる。配列はネイティブの多次元配列で扱う。
! 入力はNumPy標準の .npy 形式。I/O や行列演算は手続きに分けてある (主眼は並列化)。

module mlp
contains
  ! --- .npy を読み, 中身を「保存順 (C順) のまま」flat な real(8) 配列 a(1:n) に入れる ---
  subroutine read_npy(path, a, shp, ndim)
    character(*), intent(in) :: path
    real(8), allocatable, intent(out) :: a(:)
    integer, intent(in)  :: ndim            ! 期待する次元数 (1 or 2)
    integer, intent(out) :: shp(ndim)        ! 形 (ndim 個)
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

  ! --- 1枚の画像を MLP に通して予測クラス(0..9)を返す ---
  function predict(W1, b1, W2, b2, x) result(cls)
    real(8), intent(in) :: W1(:,:), b1(:), W2(:,:), b2(:), x(:)
    integer :: cls
    real(8) :: h(size(W1,2)), o(size(W2,2))
    call matvec(W1, b1, x, h)            ! h = W1 x + b1
    h = max(0.0d0, h)                    ! ReLU
    call matvec(W2, b2, h, o)            ! o = W2 h + b2
    cls = maxloc(o, 1) - 1               ! argmax (クラスは 0 始まり)
  end function predict
end module mlp

program mnist_infer
  use mlp
  use omp_lib
  implicit none
  integer :: IN, HID, OUT, NT, i, sh1(1), sh2(2)
  integer(8) :: correct
  real(8) :: t0, elapsed
  real(8), allocatable :: W1(:,:), b1(:), W2(:,:), b2(:), X(:,:), flat(:)
  integer, allocatable :: y(:)

  ! 重みの読み込み (.npy は C順なので reshape で W(j,k)=W[k][j] の列優先並びになる)
  call read_npy("data/W1.npy", flat, sh2, 2); HID = sh2(1); IN = sh2(2)
  W1 = reshape(flat, [IN, HID])
  call read_npy("data/b1.npy", flat, sh1, 1); b1 = flat
  call read_npy("data/W2.npy", flat, sh2, 2); OUT = sh2(1)
  W2 = reshape(flat, [HID, OUT])
  call read_npy("data/b2.npy", flat, sh1, 1); b2 = flat

  ! テスト画像の読み込み (画素 0..255 -> 0..1), X(:,i) が画像 i
  call read_npy("data/x_test.npy", flat, sh2, 2); NT = sh2(1)
  X = reshape(flat, [IN, NT]) / 255.0d0
  call read_npy("data/y_test.npy", flat, sh1, 1)
  allocate(y(NT)); y = nint(flat)

  ! 推論: 各画像の予測クラスと正解を比べ正解数を数える。各画像は独立。
  correct = 0
  t0 = omp_get_wtime()
  ! BEGIN ANSWER
  !$omp parallel do private(i) reduction(+:correct)
  ! END ANSWER
  do i = 1, NT
     if (predict(W1, b1, W2, b2, X(:,i)) == y(i)) correct = correct + 1
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,f0.2,a)", &
       "MNIST テスト ", NT, " 枚: 正解 ", correct, " 枚, 正解率 = ", &
       100.0d0 * correct / NT, " %"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program mnist_infer
