! 本物の MNIST 手書き数字を, 学習済みの2層MLPで認識する (推論=forward)。
! 重みは 02_mlp_train が学習して書き出したもの (784->128->10):
!   data/W1.npy, b1.npy, W2.npy, b2.npy : 学習済みの重み (float64)
!   data/x_test.npy : テスト画像 (uint8 [N,784], 画素 0..255)
!   data/y_test.npy : 正解ラベル (int32 [N], 0..9)
! 推論の中身は「行列ベクトル積 + 活性化(ReLU) + argmax」。各画像の推論は独立なので並列化できる。
! 入力はNumPy標準の .npy 形式。read_npy は下のモジュールに用意してある (I/O は主眼ではない)。

module npy_io
contains
  ! .npy を読み, 中身を「保存順 (C順) のまま」flat な real(8) 配列 a(1:n) に入れる。
  ! 形は s0,s1 (1次元なら ndim=1, s1=1)。dtype は f8/f4/u1/i4/i8 に対応。
  subroutine read_npy(path, a, s0, s1, ndim)
    character(*), intent(in) :: path
    real(8), allocatable, intent(out) :: a(:)
    integer, intent(out) :: s0, s1, ndim
    integer :: u, hlen, p1, p2, ios
    integer(8) :: n
    character(len=10) :: magic
    character(len=:), allocatable :: hdr, sub
    character(len=8) :: descr
    integer(1), allocatable :: t1(:)
    integer(4), allocatable :: t4(:)
    integer(8), allocatable :: t8(:)
    real(4),    allocatable :: r4(:)
    integer(8) :: i

    open(newunit=u, file=path, access='stream', form='unformatted', &
         status='old', action='read')
    read(u) magic
    if (magic(1:6) /= char(147)//'NUMPY') stop '.npy ではありません'
    hlen = ichar(magic(9:9)) + 256*ichar(magic(10:10))
    allocate(character(len=hlen) :: hdr)
    read(u) hdr
    ! dtype
    if      (index(hdr,"'<f8'") > 0) then; descr = '<f8'
    else if (index(hdr,"'<f4'") > 0) then; descr = '<f4'
    else if (index(hdr,"'|u1'") > 0) then; descr = '|u1'
    else if (index(hdr,"'<i4'") > 0) then; descr = '<i4'
    else if (index(hdr,"'<i8'") > 0) then; descr = '<i8'
    else; stop '未対応の dtype'; end if
    ! shape: '(' と ')' の間を取り出し, カンマを空白にして読む
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
end module npy_io

program mnist_infer
  use npy_io
  use omp_lib
  implicit none
  integer :: IN, HID, OUT, NT, i, hh, oo, k, best, s0, s1, nd
  integer(8) :: correct
  real(8) :: s, bestv, t0, elapsed
  real(8), allocatable :: W1(:,:), b1(:), W2(:,:), b2(:), X(:,:), hidv(:)
  real(8), allocatable :: flat(:)
  integer, allocatable :: y(:)

  ! --- 重みの読み込み (.npy は C順なので reshape で Fortran の (k,hh) 並びになる) ---
  call read_npy("data/W1.npy", flat, s0, s1, nd); HID = s0; IN = s1
  W1 = reshape(flat, [IN, HID])              ! W1(k,hh) = W1[hh][k]
  call read_npy("data/b1.npy", flat, s0, s1, nd); b1 = flat
  call read_npy("data/W2.npy", flat, s0, s1, nd); OUT = s0
  W2 = reshape(flat, [HID, OUT])             ! W2(hh,oo) = W2[oo][hh]
  call read_npy("data/b2.npy", flat, s0, s1, nd); b2 = flat

  ! --- テスト画像の読み込み (画素 0..255 -> 0..1) ---
  call read_npy("data/x_test.npy", flat, s0, s1, nd); NT = s0
  X = reshape(flat, [IN, NT]) / 255.0d0      ! X(k,i) = 画像 i の画素 k
  call read_npy("data/y_test.npy", flat, s0, s1, nd)
  allocate(y(NT)); y = nint(flat)

  ! --- 推論 ---
  correct = 0
  t0 = omp_get_wtime()
  ! BEGIN ANSWER: 各画像の推論は独立。!$omp parallel do reduction(+:correct) で並列化せよ.
  !$omp parallel do private(hidv,s,best,bestv,hh,oo,k) reduction(+:correct)
  ! END ANSWER
  do i = 1, NT
     allocate(hidv(HID))
     do hh = 1, HID                       ! h = ReLU(W1 x + b1)
        s = b1(hh)
        do k = 1, IN
           s = s + W1(k,hh) * X(k,i)
        end do
        hidv(hh) = max(0.0d0, s)
     end do
     best = 1; bestv = -1d300              ! o = W2 h + b2, argmax
     do oo = 1, OUT
        s = b2(oo)
        do hh = 1, HID
           s = s + W2(hh,oo) * hidv(hh)
        end do
        if (s > bestv) then
           bestv = s; best = oo
        end if
     end do
     if (best - 1 == y(i)) correct = correct + 1   ! クラスは 0..9, best は 1..10
     deallocate(hidv)
  end do
  ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  !$omp end parallel do
  ! END ANSWER
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,f0.2,a)", &
       "MNIST テスト ", NT, " 枚: 正解 ", correct, " 枚, 正解率 = ", &
       100.0d0 * correct / NT, " %"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program mnist_infer
