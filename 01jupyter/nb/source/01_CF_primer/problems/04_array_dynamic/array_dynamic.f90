program array_dynamic
  character(len=32) :: arg
  integer(8) :: n, i
  real(8), allocatable :: a(:)
  real(8) :: s
  ! 要素数 n を実行時 (コマンドライン引数) で決める
  n = 100
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  ! TODO: a に n 要素分の領域を確保せよ (allocate).
  ! BEGIN ANSWER
  allocate(a(n))
  ! END ANSWER
  do i = 1, n
     a(i) = 1.0d0 / i        ! 1/1, 1/2, 1/3, ...
  end do
  s = 0.0d0
  do i = 1, n
     s = s + a(i)
  end do
  print "(a,i0,a,f0.6)", "sum of 1/k (k=1..", n, ") = ", s
  deallocate(a)             ! 確保した領域は解放する
end program array_dynamic
