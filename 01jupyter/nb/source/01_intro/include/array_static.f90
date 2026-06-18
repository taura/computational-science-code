program array_static
  ! 要素数を固定した(静的な)配列. 添字は既定で 1 から n
  integer, parameter :: n = 5
  real(8) :: a(n)
  real(8) :: s
  integer :: i
  do i = 1, n
     a(i) = (i - 1) * (i - 1)   ! C版の a[i]=i*i に合わせ 0,1,4,9,16
  end do
  s = 0.0d0
  do i = 1, n
     print "(a,i0,a,f0.6)", "a(", i, ") = ", a(i)
     s = s + a(i)
  end do
  print "(a,f0.6)", "sum = ", s
end program array_static
