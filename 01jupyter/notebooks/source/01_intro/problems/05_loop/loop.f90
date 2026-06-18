program loop
  character(len=32) :: arg
  integer(8) :: N, p
  integer :: k
  N = 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  ! 2 を何回かけたら N を超えるか (2^k > N となる最小の k) を求める
  p = 1            ! p = 2^k
  k = 0
  ! TODO: p が N を超えるまで「p を 2 倍し k を 1 増やす」を繰り返す do while ループを書け.
  print "(a,i0,a,i0,a,i0)", "2^", k, " = ", p, " is the first power of 2 greater than ", N
end program loop
