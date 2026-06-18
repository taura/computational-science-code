program fileio
  integer :: i, ios
  real(8) :: x, sum
  ! まず data.txt に 0..4 の i と i*0.5 を書き出す (この部分は完成済み)
  open(unit=10, file="data.txt", status="replace", action="write")
  do i = 0, 4
     write(10, "(i0,1x,f0.6)") i, i * 0.5d0
  end do
  close(10)

  ! 次に data.txt を読み直し, 2列目 (x) の合計を求める
  open(unit=10, file="data.txt", status="old", action="read")
  sum = 0.0d0
  ! TODO: read で1行ずつ (i, x) を読み (iostat で終端判定), x を sum に足し込むループを書け.
  close(10)
  print "(a,f0.6)", "sum of x = ", sum
end program fileio
