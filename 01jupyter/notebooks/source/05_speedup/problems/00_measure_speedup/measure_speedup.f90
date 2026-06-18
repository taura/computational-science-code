module lin_rec_mod
contains
  ! x = ax + b をひたすら n 回繰り返す.
  ! (|a| < 1.0 なら c によらず, x = b / (1 - a) に収束).
  ! n 回 mul + add を行う (-> 2 n flops)
  function lin_rec(a, b, c, n) result(x)
    real(8), intent(in) :: a, b, c
    integer(8), intent(in) :: n
    real(8) :: x
    integer(8) :: j
    x = c
    do j = 1, n
       x = a * x + b
    end do
  end function lin_rec
end module lin_rec_mod

program measure_speedup
  use omp_lib
  use lin_rec_mod
  character(len=32) :: arg
  integer(8) :: m, n, i
  real(8), allocatable :: x(:)
  real(8) :: t0, t1, dt, flops
  m = 64
  n = 10 * 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) n
  end if
  allocate(x(m))
  print "(a,i0,a,i0)", "m = ", m, ", n = ", n
  ! 計測開始
  t0 = omp_get_wtime()
  ! 計算本体
  ! TODO: 下の do ループを !$omp parallel do ... !$omp end parallel do で囲み, ループを並列化せよ.
  do i = 1, m
     x(i) = lin_rec(0.99d0, real(i, 8), 1.0d0, n)
  end do
  ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
  ! 計測終了
  t1 = omp_get_wtime()
  dt = t1 - t0                  ! sec
  ! 答え表示 (x(i) = 100 * i くらいのはず)
  do i = 1, m
     print "(a,i3,a,f9.3)", "x[", i, "] = ", x(i)
  end do
  flops = 2.d0 * real(m, 8) * real(n, 8)
  print "(a,f7.3,a)", "elapsed    : ", dt, "  sec"
  print "(a,es9.2)",  "flops      : ", flops
  print "(f7.3,a)", flops / dt * 1e-9, " GFLOPS"
end program measure_speedup
