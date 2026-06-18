module lin_rec_mod
contains
  ! x = ax + b をひたすら n 回繰り返す.
  ! (|a| < 1.0 なら c によらず, x = b / (1 - a) に収束).
  ! n 回 mul + add を行う (-> 2 n flops)
  !$omp declare target
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

program gpu_speedup
  use omp_lib
  use lin_rec_mod
  character(len=32) :: arg, env
  integer(8) :: m, n, i
  integer :: nteams, nthreads, st
  integer(8) :: err
  real(8), allocatable :: x(:)
  real(8) :: t0, t1, dt, flops
  ! チーム数・スレッド数を環境変数から取得
  nteams = 1
  call get_environment_variable("OMP_NUM_TEAMS", env, status=st)
  if (st == 0) read (env, *) nteams
  nthreads = 1
  call get_environment_variable("OMP_NUM_THREADS", env, status=st)
  if (st == 0) read (env, *) nthreads
  m = int(nteams, 8) * int(nthreads, 8)
  n = 100 * 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) n
  end if
  allocate(x(m))
  print "(a,i0,a,i0)", "num_teams = ", nteams, ", num_threads = ", nthreads
  print "(a,i0,a,i0)", "m = ", m, ", n = ", n
  ! 計測開始
  t0 = omp_get_wtime()
  ! 計算本体. 現状では指示行が無いのでCPU上で逐次に実行される.
  ! TODO: 下の do ループを !$omp target teams distribute parallel do num_teams(nteams) num_threads(nthreads) map(tofrom: x) ... !$omp end target teams distribute parallel do で囲み, ループをGPU上で並列実行させよ. (結果 x をCPUに戻して検算するので map(tofrom: x) が必要)
  do i = 1, m
     x(i) = lin_rec(0.99d0, real(i, 8), 1.0d0, n)
  end do
  ! TODO: 上で始めた target teams distribute parallel do 領域を閉じる (!$omp end target teams distribute parallel do).
  ! 計測終了
  t1 = omp_get_wtime()
  dt = t1 - t0                  ! sec
  ! 答え確認 (x(i) = 100 * i くらいのはず)
  err = 0
  do i = 1, m
     if (abs(x(i) - 100 * i) > 1.0d-3) then
        print "(a,i3,a,f9.3)", "x[", i, "] = ", x(i)
        err = err + 1
     end if
  end do
  if (err == 0) print "(a)", "OK"
  flops = 2.d0 * real(m, 8) * real(n, 8)
  print "(a,f7.3,a)", "elapsed    : ", dt, "  sec"
  print "(a,f7.3,a)", "elapsed/m  : ", dt / m * 1e3, " msec"
  print "(a,f7.3,a)", "elapsed/n  : ", dt / n * 1e9, " nsec"
  print "(a,f7.3,a)", "elapsed/mn : ", dt / (m * n) * 1e9, " nsec"
  print "(a,es9.2)",  "flops      : ", flops
  print "(f7.3,a)", flops / dt * 1e-9, " GFLOPS"
end program gpu_speedup
