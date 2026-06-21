! 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
!   data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
!   data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
!   data/y_test.npy : 正解ラベル (int32 [N], 0..9)
! 推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」(下の matvec / predict)。
! ネットワークの大きさは定数なので, 重み・テスト画像・ラベルを固定サイズ配列で
! 派生型 net_t にまとめる。matvec は assumed-shape で任意サイズに1つで効く。
! net_t は allocatable 成分を持たないので GPU 発展で map(to: net) がそのまま使える。

module mlp
  implicit none
  integer, parameter :: IN = 784, HID = 128, OUT = 10
  integer, parameter :: MAXN = 10000          ! テスト画像の最大枚数 (data はこれ以下)
  type :: net_t
     real(8) :: W1(IN,HID), b1(HID), W2(HID,OUT), b2(OUT)   ! 学習済みの重み
     real(8) :: x(IN,MAXN), y(MAXN)                         ! テスト画像(列が1枚)と正解ラベル
  end type net_t
contains
  ! --- .npy を読み, 任意の数値型を double として dst に直接書き込む (C順) ---
  ! 次元数 ndim は呼び出し側が指定し, ファイルの次元が違えばエラー。形を shp(1..ndim) に返す。
  subroutine read_npy(path, dst, shp, ndim)
    character(*), intent(in) :: path
    real(8), intent(out) :: dst(*)            ! 呼び出し側の配列に直接書く (sequence association)
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

  ! --- read_npy + 形のチェックだけ。形が (n0,n1) と一致するか確認 (n1=0 なら1次元,
  !     n0<0 なら行数は不問)。実際の行数を返す。 ---
  function load_npy(path, dst, n0, n1) result(nrows)
    character(*), intent(in) :: path
    real(8), intent(out) :: dst(*)
    integer, intent(in)  :: n0, n1
    integer :: nrows, shp(2), ndim
    ndim = merge(2, 1, n1 > 0)
    call read_npy(path, dst, shp(1:ndim), ndim)
    if (n0 >= 0 .and. shp(1) /= n0) then
       print "(a,a,i0,a,i0,a)", trim(path), ": 行数が想定 ", n0, " と違います (", shp(1), ")"; stop 1
    end if
    if (n1 > 0 .and. shp(2) /= n1) then
       print "(a,a,i0,a,i0,a)", trim(path), ": 列数が想定 ", n1, " と違います (", shp(2), ")"; stop 1
    end if
    nrows = shp(1)
  end function load_npy

  ! --- 行列ベクトル積 + バイアス: y = W x + b。W(n_in,n_out), assumed-shape で任意サイズに対応 ---
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

  ! --- i 番目の画像 (net%x(:,i)) を MLP に通して予測クラス(0..9)を返す ---
  function predict(net, i) result(cls)
    type(net_t), intent(in) :: net
    integer, intent(in) :: i
    integer :: cls
    real(8) :: h(HID), o(OUT)
    call matvec(net%W1, net%b1, net%x(:,i), h)
    h = max(0.0d0, h)
    call matvec(net%W2, net%b2, h, o)
    cls = maxloc(o, 1) - 1               ! argmax (クラスは 0 始まり)
  end function predict
end module mlp

program mnist_infer
  use mlp
  use omp_lib
  implicit none
  type(net_t), save :: net          ! 画像も含み大きいので静的領域に置く (スタック回避)
  integer :: NT, i

  ! 重み・テスト画像・ラベルを Net に直接読み込む
  i = load_npy("data/W1.npy", net%W1, HID, IN)
  i = load_npy("data/b1.npy", net%b1, HID, 0)
  i = load_npy("data/W2.npy", net%W2, OUT, HID)
  i = load_npy("data/b2.npy", net%b2, OUT, 0)
  NT = load_npy("data/x_test.npy", net%x, -1, IN)        ! 枚数は不問
  i  = load_npy("data/y_test.npy", net%y, NT, 0)
  net%x(:, 1:NT) = net%x(:, 1:NT) / 255.0d0              ! 0..255 -> 0..1

  block
    integer(8) :: correct
    real(8) :: t0, elapsed
    correct = 0
    t0 = omp_get_wtime()
    ! BEGIN ANSWER
    !$omp parallel do private(i) reduction(+:correct)
    ! END ANSWER
    do i = 1, NT
       if (predict(net, i) == nint(net%y(i))) correct = correct + 1
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
    elapsed = omp_get_wtime() - t0
    print "(a,i0,a,i0,a,f0.2,a)", &
         "MNIST テスト ", NT, " 枚: 正解 ", correct, " 枚, 正解率 = ", &
         100.0d0 * correct / NT, " %"
    print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
  end block
end program mnist_infer
