program types
  integer(4) :: i = 42                      ! 4バイト整数
  integer(8) :: l = 10000000000_8           ! 8バイト整数
  real(4)    :: f = 3.14                     ! 単精度 (32 bit)
  real(8)    :: d = 3.141592653589793d0      ! 倍精度 (64 bit)

  print "(a,i0)",     "i = ", i
  print "(a,i0)",     "l = ", l
  print "(a,f0.6)",   "f = ", f
  print "(a,f0.15)",  "d = ", d

  ! それぞれが何バイトかを表示する
  print "(a,i0,a)", "storage_size(i)/8 = ", storage_size(i) / 8, " bytes"
  print "(a,i0,a)", "storage_size(l)/8 = ", storage_size(l) / 8, " bytes"
  print "(a,i0,a)", "storage_size(f)/8 = ", storage_size(f) / 8, " bytes"
  print "(a,i0,a)", "storage_size(d)/8 = ", storage_size(d) / 8, " bytes"

  ! 整数同士の割り算は切り捨て, 実数の割り算は普通の割り算
  print "(a,i0)",   "7 / 2     = ", 7 / 2
  print "(a,f0.6)", "7.0 / 2.0 = ", 7.0d0 / 2.0d0
end program types
