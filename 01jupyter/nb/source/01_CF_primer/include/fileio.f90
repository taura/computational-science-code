program fileio
  integer :: i, ios
  real(8) :: x
  ! 書き込み: unit 番号を割り当てて開く
  open(unit=10, file="data.txt", status="replace", action="write")
  do i = 0, 4
     write(10, "(i0,1x,f0.6)") i, i * 0.5d0   ! 1行に2つの数を書く
  end do
  close(10)

  ! 読み込み
  open(unit=10, file="data.txt", status="old", action="read")
  do
     read(10, *, iostat=ios) i, x      ! iostat /= 0 で読み終わり
     if (ios /= 0) exit
     print "(a,i0,a,f0.6)", "read: i = ", i, ", x = ", x
  end do
  close(10)
end program fileio
