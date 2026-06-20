! 2D 波動方程式 u_tt = c^2 (u_xx + u_yy) を陽解法で時間発展させる。
! L×L の膜。四辺を固定 (u=0) し, 中央に山(ガウス)を置いて波が広がり反射する様子を計算する。
! 更新式 (5点ラプラシアン, coef=(c*dt/dx)^2, 安定条件 coef<=0.5):
!   u^{n+1} = 2 u^n - u^{n-1} + coef*(上+下+左+右 - 4*中央)
! 時間方向は前後ステップに依存するので逐次, 空間の二重ループを並列化する。
program wave
  use omp_lib
  implicit none
  integer :: L, steps, t, i, j
  real(8) :: coef, c0, sig, r2, v, lap, maxabs, asym, s, a, t0, elapsed
  real(8), allocatable, target :: arr1(:,:), arr2(:,:), arr3(:,:)
  real(8), pointer :: up(:,:), cu(:,:), nx(:,:), tmp(:,:)
  character(len=32) :: arg
  L = 257; steps = 200; coef = 0.25d0
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) L
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) steps
  end if

  allocate(arr1(0:L-1,0:L-1), arr2(0:L-1,0:L-1), arr3(0:L-1,0:L-1))
  arr1 = 0.0d0; arr2 = 0.0d0; arr3 = 0.0d0
  up => arr1; cu => arr2; nx => arr3
  ! 初期条件: 中央にガウスの山, 初速 0 (up と cu を同じに)。境界は 0 のまま。
  c0 = (L - 1) / 2.0d0; sig = L / 16.0d0
  do j = 1, L - 2
     do i = 1, L - 2
        r2 = (i - c0)**2 + (j - c0)**2
        v = exp(-r2 / (2.0d0 * sig * sig))
        up(i,j) = v
        cu(i,j) = v
     end do
  end do

  t0 = omp_get_wtime()
  do t = 1, steps
     ! 内部の各点を更新 (時間1ステップ進める)
     ! BEGIN ANSWER: 内側の二重ループを !$omp parallel do collapse(2) private(lap) で並列化せよ.
     !$omp parallel do collapse(2) private(lap)
     ! END ANSWER
     do j = 1, L - 2
        do i = 1, L - 2
           lap = cu(i-1,j) + cu(i+1,j) + cu(i,j-1) + cu(i,j+1) - 4.0d0 * cu(i,j)
           nx(i,j) = 2.0d0 * cu(i,j) - up(i,j) + coef * lap
        end do
     end do
     ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do).
     !$omp end parallel do
     ! END ANSWER
     ! up <- cu <- nx と時間を1つ進める (ポインタを回す)
     tmp => up; up => cu; cu => nx; nx => tmp
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算1: 最大振幅。検算2: i<->j 対称性の誤差 (≈0 なら正しい)。
  maxabs = 0.0d0; asym = 0.0d0
  do j = 0, L - 1
     do i = 0, L - 1
        a = abs(cu(i,j))
        if (a > maxabs) maxabs = a
        s = abs(cu(i,j) - cu(j,i))
        if (s > asym) asym = s
     end do
  end do
  print "(a,i0,a,i0,a,f0.4,a,es9.2,a)", &
       "L=", L, ", steps=", steps, ": 最大振幅=", maxabs, ", 対称性誤差=", asym, " (≈0 なら正しい)"
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program wave
