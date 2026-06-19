! 注: ベクトル型 (vector_size) は C/C++ 独自の拡張で Fortran には無い.
! Fortran では普通のループ (または配列演算 y = 2*x + 1) を書けば,
! コンパイラが自動的にSIMD化してくれる.
program map_kernel
  character(len=32) :: arg
  integer(8) :: n, i, err
  real(8), allocatable :: x(:), y(:)
  n = 64
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(x(n), y(n))
  do i = 1, n
     x(i) = i - 1
  end do

  ! y(i) = 2*x(i) + 1 を計算する
  do i = 1, n
     ! BEGIN ANSWER: y(i) = 2*x(i) + 1 を計算せよ (Fortran はこの配列演算を自動でSIMD化する).
     y(i) = 2.0d0 * x(i) + 1.0d0
     ! END ANSWER
  end do

  err = 0
  do i = 1, n
     if (y(i) /= 2.0d0 * x(i) + 1.0d0) err = err + 1
  end do
  if (err == 0) then
     print "(a,f0.1,a,i0,a,f0.1)", "OK: y(1)=", y(1), ", y(", n, ")=", y(n)
  else
     print "(a,i0,a)", "NG: ", err, " 要素が不正"
  end if
end program map_kernel
