! 注: C/C++ のベクトル型 (vector_size) による「nl 本を明示的にまとめて進める」技法は
! Fortran には無い. ここでは m 本の独立な漸化式を素直なループで書き, コンパイラの
! 自動ベクトル化・命令レベル並列に任せる (どこまで速くなるかは nvfortran 次第).
program tune_nl
  character(len=32) :: arg
  integer(8) :: m, n, i, j, c0, c1, rate
  real(8) :: t, s, dt, flops
  m = 8
  n = 100_8 * 1000 * 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) m
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) n
  end if
  s = 0.0d0
  call system_clock(c0, rate)
  do i = 1, m
     t = 1.0d0
     do j = 1, n
        t = 0.99d0 * t + 1.0d0
     end do
     s = s + t
  end do
  call system_clock(c1)
  dt = real(c1 - c0, 8) / real(rate, 8)
  flops = 2.0d0 * real(m, 8) * real(n, 8)
  print "(a,i0,a,i0,a,f0.3,a,f0.6,a)", &
       "m=", m, ", n=", n, " : ", flops / dt * 1e-9, " GFLOPS (s=", s, ")"
end program tune_nl
