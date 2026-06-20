module mlp_mod
  integer, parameter :: IN = 784   ! 入力次元
  integer, parameter :: HID = 128  ! 隠れ層のニューロン数
  integer, parameter :: OUTC = 10  ! 出力クラス数
contains
  ! 状態を持たない乱数 (重み・入力の生成用): (seed,k) から [0,1)。
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
end module mlp_mod

! MNIST を模した 2層 MLP の推論 (forward):
! 入力 784 → 隠れ 128 (ReLU) → 出力 10。
! h = ReLU(W1 x + b1) (128次元), o = W2 h + b2 (10次元), 予測 = argmax(o)。
! ニューラルネットの推論の正体は「行列積 + 活性化関数」であり,
! これまで並列化してきた行列(ベクトル)積がそのまま AI の推論になる。
! 重みは乱数 (学習済みパラメータの代わり) なので予測の中身に意味はないが,
! 計算の流れ (行列積 + ReLU + argmax) は本物である。
program mnist_infer
  use mlp_mod
  use omp_lib
  character(len=32) :: arg
  integer :: B, R, rep, n, j, c, k, amax, show
  real(8) :: s, checksum, t0, elapsed
  real(8), allocatable :: W1(:), b1(:), W2(:), b2(:), X(:)
  integer, allocatable :: pred(:)
  integer(8) :: i8

  B = 64; R = 2000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) B
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) R
  end if

  ! 重み・バイアスを乱数で生成 (小さい値 [-0.05, 0.05) 付近)。添字は 0 始まり。
  allocate(W1(0:HID*IN-1), b1(0:HID-1), W2(0:OUTC*HID-1), b2(0:OUTC-1))
  allocate(X(0:int(B,8)*IN-1), pred(0:B-1))
  do i8 = 0, int(HID,8)*IN - 1
     W1(i8) = (draw_rand01(i8, 1_8) - 0.5d0) * 0.1d0
  end do
  do i8 = 0, HID - 1
     b1(i8) = (draw_rand01(i8, 2_8) - 0.5d0) * 0.1d0
  end do
  do i8 = 0, int(OUTC,8)*HID - 1
     W2(i8) = (draw_rand01(i8, 3_8) - 0.5d0) * 0.1d0
  end do
  do i8 = 0, OUTC - 1
     b2(i8) = (draw_rand01(i8, 4_8) - 0.5d0) * 0.1d0
  end do
  do i8 = 0, int(B,8)*IN - 1
     X(i8) = draw_rand01(i8, 5_8)
  end do

  t0 = omp_get_wtime()
  do rep = 1, R
     checksum = 0.0d0
     ! 各画像の forward は互いに独立。バッチを分担して並列に推論する。
     ! BEGIN ANSWER: バッチ (画像) のループを !$omp parallel do で並列化せよ (per-image の作業を block 内のローカル配列で行い自動的に private にする).
     !$omp parallel do private(n, j, c, k, s, amax) reduction(+:checksum)
     ! END ANSWER
     do n = 0, B - 1
        block
          real(8) :: h(0:HID-1), o(0:OUTC-1)
          ! 1層目: h = ReLU(W1 x + b1) (行列ベクトル積 + ReLU)
          do j = 0, HID - 1
             s = b1(j)
             do k = 0, IN - 1
                s = s + W1(int(j,8)*IN + k) * X(int(n,8)*IN + k)
             end do
             if (s > 0.0d0) then
                h(j) = s
             else
                h(j) = 0.0d0
             end if
          end do
          ! 2層目: o = W2 h + b2 (行列ベクトル積)
          do c = 0, OUTC - 1
             s = b2(c)
             do k = 0, HID - 1
                s = s + W2(int(c,8)*HID + k) * h(k)
             end do
             o(c) = s
             checksum = checksum + s
          end do
          ! 予測クラス = argmax(o)
          amax = 0
          do c = 1, OUTC - 1
             if (o(c) > o(amax)) amax = c
          end do
          pred(n) = amax
        end block
     end do
     ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     !$omp end parallel do
     ! END ANSWER
  end do
  elapsed = omp_get_wtime() - t0

  show = min(B, 8)
  write (*, '(a,i0,a,i0,a,i0,a)', advance='no') &
       "batch=", B, ", hidden=", HID, ": 予測クラス[0..", show-1, "]="
  do n = 0, show - 1
     if (n < show - 1) then
        write (*, '(i0,a)', advance='no') pred(n), ","
     else
        write (*, '(i0)', advance='no') pred(n)
     end if
  end do
  write (*, '(a,f0.6)') ", checksum=", checksum
  print "(a)", "(結果は OMP_NUM_THREADS によらず一致する: 各画像は固定順序で独立に計算)"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program mnist_infer
