program gpu_integral
  character(len=32) :: arg
  integer(8) :: n, i
  real(8) :: dx, s, x, pi
  real(8), parameter :: pi_ref = 3.141592653589793d0
  n = 100000000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  dx = 1.0d0 / dble(n)
  s = 0.0d0

  ! 中点則で ∫_0^1 4/(1+x^2) dx = π を GPU 上で計算する.
  ! s は総和 (スカラ) なので reduction(+:s) を使う.
  ! スカラはコンパイラが自動的に転送するので map は不要.
  ! BEGIN ANSWER
  !$omp target teams distribute parallel do reduction(+:s) private(x)
  do i = 0, n - 1
     x = (i + 0.5d0) * dx
     s = s + 4.0d0 / (1.0d0 + x * x)
  end do
  !$omp end target teams distribute parallel do
  ! END ANSWER

  pi = s * dx
  print "(a,i0)", "n = ", n
  print "(a,f0.15)", "pi  = ", pi
  print "(a,f0.15)", "M_PI = ", pi_ref
  print "(a,es10.3)", "error = ", abs(pi - pi_ref)
end program gpu_integral
