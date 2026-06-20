module mlp_mod
contains
  ! 状態を持たない乱数 (合成データ・初期値生成用): (seed,k) から [0,1)。
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

  function sigmoidf(z) result(s)
    real(8), intent(in) :: z
    real(8) :: s
    s = 1.0d0 / (1.0d0 + exp(-z))
  end function sigmoidf
end module mlp_mod

! 多層パーセプトロン (MLP) を自分で学習させる。
! ネットワーク: 入力 2 -> 隠れ層 H (tanh) -> 出力 1 (sigmoid)。
! 2次元データの二値分類。境界が「円」(非線形分離) なので隠れ層が必須。
! forward -> 損失 -> backprop -> 更新 を繰り返す。
! 並列化対象は「全サンプルにわたる勾配の和」(配列 reduction)。
program mlp_train
  use mlp_mod
  use omp_lib
  character(len=32) :: arg
  integer, parameter :: D = 2
  integer :: H, E, ep, k, d2
  integer(8) :: N8, i
  real(8) :: lr, R2, loss, x0, x1, o_in, o, yi, dout, dh, z, hk, s, gb2, b2, elapsed, t0
  integer(8) :: correct
  real(8), allocatable :: X(:), W1(:), b1(:), W2(:), gW1(:), gb1(:), gW2(:), hh(:)
  integer, allocatable :: y(:)

  N8 = 4000; H = 32; E = 3000; lr = 0.7d0; R2 = 0.5d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N8
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) H
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) E
  end if
  if (command_argument_count() >= 4) then
     call get_command_argument(4, arg); read (arg, *) lr
  end if

  ! 合成データ: [-1,1]^2 上の点。原点に近ければ class 1。非線形分離。
  allocate(X(0:N8*D-1), y(0:N8-1))
  do i = 0, N8 - 1
     x0 = draw_rand01(i, 0_8) * 2.0d0 - 1.0d0
     x1 = draw_rand01(i, 1_8) * 2.0d0 - 1.0d0
     X(i*D+0) = x0; X(i*D+1) = x1
     if (x0*x0 + x1*x1 < R2) then
        y(i) = 1
     else
        y(i) = 0
     end if
  end do

  ! パラメータ: W1[H,D], b1[H], W2[H], b2。小さな乱数で初期化。
  allocate(W1(0:H*D-1), b1(0:H-1), W2(0:H-1), gW1(0:H*D-1), gb1(0:H-1), gW2(0:H-1))
  do k = 0, H - 1
     do d2 = 0, D - 1
        W1(k*D+d2) = draw_rand01(int(k,8), int(d2+10,8)) - 0.5d0
     end do
     b1(k) = 0.0d0
     W2(k) = draw_rand01(int(k,8), 99_8) - 0.5d0
  end do
  b2 = 0.0d0

  loss = 0.0d0; correct = 0
  t0 = omp_get_wtime()
  do ep = 0, E - 1
     gW1 = 0.0d0; gb1 = 0.0d0; gW2 = 0.0d0; gb2 = 0.0d0
     loss = 0.0d0; correct = 0

     ! 全サンプルにわたる forward + backprop。各サンプルの勾配寄与を総和する。
     ! 損失・正解数はスカラ reduction, 勾配は配列 reduction で競合を避ける。
     ! TODO: サンプルのループを配列 reduction で並列化せよ: !$omp parallel do private(...) reduction(+:loss,correct,gb2,gW1,gb1,gW2) (h は private)。
     do i = 0, N8 - 1
        allocate(hh(0:H-1))
        x0 = X(i*D+0); x1 = X(i*D+1)
        ! forward
        o_in = b2
        do k = 0, H - 1
           z = b1(k) + W1(k*D+0)*x0 + W1(k*D+1)*x1
           hk = tanh(z)
           hh(k) = hk
           o_in = o_in + W2(k) * hk
        end do
        o = sigmoidf(o_in)
        yi = real(y(i), 8)
        if (y(i) == 1) then
           loss = loss - log(o + 1.0d-12)
        else
           loss = loss - log(1.0d0 - o + 1.0d-12)
        end if
        if (merge(1, 0, o > 0.5d0) == y(i)) correct = correct + 1
        ! backprop
        dout = o - yi
        gb2 = gb2 + dout
        do k = 0, H - 1
           gW2(k) = gW2(k) + dout * hh(k)
           dh = dout * W2(k) * (1.0d0 - hh(k)*hh(k))
           gW1(k*D+0) = gW1(k*D+0) + dh * x0
           gW1(k*D+1) = gW1(k*D+1) + dh * x1
           gb1(k)     = gb1(k) + dh
        end do
        deallocate(hh)
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
     loss = loss / real(N8, 8)

     ! 更新 (勾配を平均して降下)
     s = lr / real(N8, 8)
     do k = 0, H - 1
        W1(k*D+0) = W1(k*D+0) - s * gW1(k*D+0)
        W1(k*D+1) = W1(k*D+1) - s * gW1(k*D+1)
        b1(k)     = b1(k) - s * gb1(k)
        W2(k)     = W2(k) - s * gW2(k)
     end do
     b2 = b2 - s * gb2

     if (mod(ep, 500) == 0 .or. ep == E - 1) then
        print "(a,i4,a,f7.4,a,f6.2,a)", "epoch ", ep, ": loss=", loss, &
             ", acc=", 100.0d0 * correct / N8, "%"
     end if
  end do
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,i0,a,f7.4,a,f6.2,a)", "最終: N=", N8, ", H=", H, &
       ", epochs=", E, ", loss=", loss, ", acc=", 100.0d0 * correct / N8, "%"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program mlp_train
