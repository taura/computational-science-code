program mandelbrot
  implicit none
  integer :: W, H, maxiter
  integer :: px, i, j, it
  integer, allocatable :: cnt(:)
  real(8) :: xmin, xmax, ymin, ymax, cx, cy, zr, zi, zr2, zi2
  integer(8) :: total
  character(len=32) :: arg

  ! 画像サイズと最大反復数
  W = 1000; H = 1000; maxiter = 1000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read(arg, *) W
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read(arg, *) H
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read(arg, *) maxiter
  end if

  allocate(cnt(0:W*H-1))

  ! 複素平面の描画範囲
  xmin = -2.0d0; xmax = 1.0d0
  ymin = -1.5d0; ymax = 1.5d0

  ! 各ピクセルの脱出反復数を計算する.
  ! 内部の点は maxiter まで回るため画素ごとの仕事量が大きく異なる (負荷が不均一).
  ! TODO: 下の px ループを !$omp parallel do schedule(dynamic) private(i, j, cx, cy, zr, zi, zr2, zi2, it) ... !$omp end parallel do で囲め. 仕事量が画素ごとに大きく異なるため, dynamic スケジュールが負荷を均す.
  ! BEGIN ANSWER
  !$omp parallel do schedule(dynamic) private(i, j, cx, cy, zr, zi, zr2, zi2, it)
  ! END ANSWER
  do px = 0, W*H - 1
     i = px / W   ! 行
     j = mod(px, W)  ! 列
     cx = xmin + (xmax - xmin) * j / (W - 1)
     cy = ymin + (ymax - ymin) * i / (H - 1)
     ! z = z^2 + c を |z|^2 > 4 か maxiter まで反復 (複素数を手で展開)
     zr = 0.0d0; zi = 0.0d0
     it = 0
     do while (it < maxiter .and. zr*zr + zi*zi <= 4.0d0)
        zr2 = zr*zr - zi*zi + cx
        zi2 = 2.0d0 * zr * zi + cy
        zr = zr2
        zi = zi2
        it = it + 1
     end do
     cnt(px) = it
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER

  ! 並列ループの後で総反復数を逐次に集計する (共有変数への足し込みによる競合を避ける)
  total = 0
  do px = 0, W*H - 1
     total = total + cnt(px)
  end do

  print "(a,i0,a,i0,a,i0)", "W=", W, " H=", H, " maxiter=", maxiter
  print "(a,i0)", "total iterations = ", total
  print "(a,i0,a,i0,a,i0)", "sample cnt: top-left=", cnt(0), &
       " center=", cnt((H/2)*W + W/2), " bottom-right=", cnt(W*H - 1)

  deallocate(cnt)
end program mandelbrot
