! 2D 定常熱伝導 (ラプラス方程式) をヤコビ反復で解く。
! L×L の板。上端(行0)=100, 残り3辺=0 に固定し, 内部が定常分布に落ち着くまで反復。
! 各内部点を上下左右4点の平均で更新する (5点ステンシル)。
program heat2d
  use omp_lib
  implicit none
  integer :: L, maxiter, iter, i, j
  real(8) :: tol, diff, val, v, d, t0, elapsed
  real(8), allocatable, target :: a(:,:), b(:,:)
  real(8), pointer :: u(:,:), unew(:,:), tmp(:,:)
  character(len=32) :: arg
  L = 129; tol = 1d-6; maxiter = 1000000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) L
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) maxiter
  end if

  allocate(a(0:L-1, 0:L-1), b(0:L-1, 0:L-1))
  ! 初期化: 上端(行0)=100, 他=0。境界はずっと固定なので両配列に同じ値を入れておく。
  do j = 0, L - 1
     do i = 0, L - 1
        val = merge(100.0d0, 0.0d0, i == 0)
        a(i, j) = val
        b(i, j) = val
     end do
  end do
  u => a; unew => b

  t0 = omp_get_wtime()
  do iter = 1, maxiter
     diff = 0.0d0   ! この反復での最大更新量
     ! 内部の各点 (i,j) を上下左右の平均で更新し, 更新量の最大値を求める。
     ! TODO: 内側の二重ループを !$omp parallel do collapse(2) private(v,d) reduction(max:diff) で並列化せよ.
     do j = 1, L - 2
        do i = 1, L - 2
           v = 0.25d0 * (u(i-1,j) + u(i+1,j) + u(i,j-1) + u(i,j+1))
           d = abs(v - u(i,j))
           if (d > diff) diff = d
           unew(i,j) = v
        end do
     end do
     ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
     ! u と unew を入れ替える (コピーせずポインタを差し替えるだけ)
     tmp => u; u => unew; unew => tmp
     if (diff < tol) exit
  end do
  elapsed = omp_get_wtime() - t0

  print "(a,i0,a,i0,a,es9.2,a,f0.4,a)", &
       "L=", L, ", iters=", min(iter, maxiter), ", 最終残差=", diff, &
       ", 中心温度=", u(L/2, L/2), " (理論 25.0)"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program heat2d
