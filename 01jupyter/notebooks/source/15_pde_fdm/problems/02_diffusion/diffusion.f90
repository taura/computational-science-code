! 2D 拡散方程式 u_t = D (u_xx + u_yy) を陽解法 (FTCS) で時間発展させる。
! 中央に置いたインクの塊が時間とともに広がる様子を計算する。
! 更新式 (5点ラプラシアン, alpha=D*dt/dx^2, 安定条件 alpha<=0.25):
!   u^{n+1}(i,j) = u + alpha*(上+下+左+右 - 4*中央)
! 境界は「反射(断熱)」: 領域外の隣は自分自身とみなす (端で添字を留める)。
! → インクが外へ漏れないので, 全体の総量は時間によらず保存される (検算に使う)。
program diffusion
  use omp_lib
  implicit none
  integer :: L, steps, t, i, j
  real(8) :: alpha, mass0, mass1, maxc, t0, elapsed
  real(8), allocatable, target :: arr1(:,:), arr2(:,:)
  real(8), pointer :: u(:,:), un(:,:), tmp(:,:)
  integer :: lo, hi
  character(len=32) :: arg
  L = 256; steps = 500; alpha = 0.2d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) L
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) steps
  end if

  allocate(arr1(0:L-1,0:L-1), arr2(0:L-1,0:L-1))
  arr1 = 0.0d0; arr2 = 0.0d0
  u => arr1; un => arr2
  ! 初期条件: 中央の正方形ブロックに濃度 1 のインクを置く。
  lo = L/2 - L/16; hi = L/2 + L/16
  do j = lo, hi - 1
     do i = lo, hi - 1
        u(i,j) = 1.0d0
     end do
  end do
  mass0 = sum(u)

  t0 = omp_get_wtime()
  do t = 1, steps
     ! 全格子点を更新 (時間1ステップ進める)。端では添字を留めて反射境界にする。
     ! TODO: 更新の二重ループを !$omp parallel do collapse(2) で並列化せよ.
     do j = 0, L - 1
        do i = 0, L - 1
           un(i,j) = u(i,j) + alpha * ( &
                u(max(i-1,0),   j) + u(min(i+1,L-1), j) + &
                u(i, max(j-1,0))   + u(i, min(j+1,L-1)) - 4.0d0 * u(i,j) )
        end do
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
     tmp => u; u => un; un => tmp
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算: 総量 (sum) が保存されているか。最大濃度は広がるほど下がる。
  mass1 = sum(u)
  maxc = maxval(u)
  print "(a,i0,a,i0,a,f0.6,a,f0.6,a,f0.6)", &
       "L=", L, ", steps=", steps, ": 総量 ", mass0, " -> ", mass1, &
       " (保存されるはず), 最大濃度 ", maxc
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program diffusion
