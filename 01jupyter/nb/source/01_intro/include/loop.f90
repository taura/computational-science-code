program loop
  integer :: i, j
  integer(8) :: p
  ! do 文: 始点, 終点 (, 増分)
  print "(a)", "do loop:"
  do i = 0, 4
     print "(a,i0)", "  i = ", i
  end do

  ! do while 文: 条件が成り立つ間繰り返す
  print "(a)", "do while loop:"
  j = 1
  p = 1
  do while (p < 100)
     p = p * 2          ! 2 の累乗
     print "(a,i0,a,i0)", "  2^", j, " = ", p
     j = j + 1
  end do
end program loop
