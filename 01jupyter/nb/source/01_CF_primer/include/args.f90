program args
  character(len=256) :: arg
  integer :: argc, i, n
  real(8) :: x
  ! command_argument_count() は引数の個数 (プログラム名を含まない)
  argc = command_argument_count()
  print "(a,i0)", "argc = ", argc
  do i = 0, argc
     call get_command_argument(i, arg)   ! i=0 はプログラム名
     print "(a,i0,a,a)", "arg(", i, ") = ", trim(arg)
  end do
  ! 文字列を数値に変換する: 内部 read を使う
  n = 10
  x = 1.0d0
  if (argc >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  if (argc >= 2) then
     call get_command_argument(2, arg); read (arg, *) x
  end if
  print "(a,i0,a,f0.6)", "n = ", n, ", x = ", x
end program args
