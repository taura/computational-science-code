! Fortran には C/C++ のベクトル型拡張 (vector_size) に相当する機能が無い.
! そこで, 内側の漸化式ループを !$omp simd で SIMD 化し,
! 外側の独立な要素のループを !$omp parallel do でマルチコア並列化して,
! SIMD + 命令レベル並列 + マルチコアをコンパイラに任せる.

module lin_rec_mod
contains
  ! t = a*t + b を n 回繰り返す
  real(8) function lin_rec(a, b, c, n) result(t)
    real(8), intent(in) :: a, b, c
    integer(8), intent(in) :: n
    integer(8) :: j
    t = c
    !$omp simd
    do j = 1, n
       t = a * t + b
    end do
  end function lin_rec
end module lin_rec_mod

program simd_multicore
  use lin_rec_mod
  use omp_lib
  implicit none
  integer(8) :: m, n, i, err
  real(8), allocatable :: x(:)
  real(8) :: t0, t1, dt, flops
  character(len=32) :: arg

  m = 8
  n = 1000_8 * 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read(arg, *) m
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read(arg, *) n
  end if
  allocate(x(m))
  print '(A,I0,A,I0)', "m = ", m, ", n = ", n

  t0 = omp_get_wtime()
  ! TODO: 下の do ループの直前に !$omp parallel do を1行追加し, 外側 (互いに独立な要素) のループをマルチコアで並列化せよ.
  ! BEGIN ANSWER
  !$omp parallel do
  ! END ANSWER
  do i = 1, m
     x(i) = lin_rec(0.99d0, dble(i), 1.0d0, n)
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  t1 = omp_get_wtime()
  dt = t1 - t0

  err = 0
  do i = 1, m
     if (abs(x(i) - 100.0d0 * dble(i)) > 1.0d-3) then
        print '(A,I3,A,F9.3)', "x(", i, ") = ", x(i)
        err = err + 1
     end if
  end do
  if (err == 0) print '(A)', "OK"

  flops = 2.0d0 * dble(m) * dble(n)
  print '(A,F7.3,A)', "elapsed    : ", dt, "  sec"
  print '(A,ES9.2)', "flops      : ", flops
  print '(F8.3,A)', flops / dt * 1d-9, " GFLOPS"
end program simd_multicore
