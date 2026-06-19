module lin_rec_mod
contains
  ! x = a*x + b を n 回繰り返す (2n flops)
  function lin_rec(a, b, c, n) result(x)
    !$omp declare target
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

program cpu_vs_gpu
  use omp_lib
  use lin_rec_mod
  character(len=32) :: arg
  integer(8) :: m, n, i
  real(8), allocatable :: x(:)
  real(8) :: t0, t1, dt, flops
  m = 1024
  n = 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) n
  end if
  allocate(x(m))

  t0 = omp_get_wtime()
  ! このループは GPU 用にオフロード指示が付いている (完成済み).
  ! OMP_TARGET_OFFLOAD=DISABLED ならホスト(CPU)で, MANDATORY ならGPUで実行される.
  !$omp target teams distribute parallel do map(tofrom: x)
  do i = 1, m
     x(i) = lin_rec(0.99d0, real(i, 8), 1.0d0, n)
  end do
  !$omp end target teams distribute parallel do
  t1 = omp_get_wtime()
  dt = t1 - t0

  flops = 2.0d0 * real(m, 8) * real(n, 8)
  print "(a,i0,a,i0,a,f0.3,a,f0.3,a)", &
       "m = ", m, ", n = ", n, ", elapsed = ", dt, " sec, ", flops / dt * 1e-9, " GFLOPS"
end program cpu_vs_gpu
