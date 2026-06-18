program array_static
  integer, parameter :: n = 10
  real(8) :: a(n)
  real(8) :: s
  integer :: i
  ! 配列を a(i) = i の二乗 で埋める (添字は 1..n)
  do i = 1, n
     a(i) = real(i, 8) * real(i, 8)
  end do
  ! 合計を求める
  s = 0.0d0
  ! BEGIN ANSWER: 配列 a の全要素を s に足し込むループを書け.
  do i = 1, n
     s = s + a(i)
  end do
  ! END ANSWER
  print "(a,i0,a,f0.0)", "sum of squares 1..", n, " = ", s
end program array_static
