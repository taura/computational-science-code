program vecadd
  character(len=32) :: arg
  integer(8) :: n, i, err
  real(8), allocatable :: a(:), b(:), c(:)
  n = 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(a(n), b(n), c(n))
  do i = 1, n
     a(i) = i; b(i) = 2 * i; c(i) = -1.0d0
  end do

  ! c(i) = a(i) + b(i) を GPU で計算する
  ! TODO: ループをGPUにオフロードして c(i)=a(i)+b(i) を計算せよ. a,b は map(to:), 結果 c は map(from:) で受け取る.

  ! 検算
  err = 0
  do i = 1, n
     if (c(i) /= a(i) + b(i)) err = err + 1
  end do
  if (err == 0) then
     print "(a,f0.0,a,i0,a,f0.0)", "OK: c(1) = ", c(1), ", c(", n, ") = ", c(n)
  else
     print "(a,i0,a)", "NG: ", err, " 要素が不正"
  end if
end program vecadd
