program args
  character(len=64) :: arg
  integer :: n, i
  real(8) :: x, p
  ! 第1引数を整数 n, 第2引数を実数 x として受け取り, x の n 乗を表示する.
  ! 引数が無いときの既定値は n=3, x=2.0
  n = 3
  x = 2.0d0
  ! TODO: 1番目の引数を n に, 2番目の引数を x に, 内部 read で変換せよ (引数があるときだけ).
  p = 1.0d0
  do i = 1, n
     p = p * x
  end do
  print "(f0.6,a,i0,a,f0.6)", x, " ^ ", n, " = ", p
end program args
