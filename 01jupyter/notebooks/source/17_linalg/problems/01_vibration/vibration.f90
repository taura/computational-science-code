module vibration_mod
contains
  ! 状態を持たない乱数 (初期ベクトルの生成用): (seed,k) から [0,1)。
  function draw_rand01(seed, k) result(u)
    integer(8), intent(in) :: seed, k
    real(8) :: u
    integer(8), parameter :: M = 2147483647_8
    integer(8) :: x
    x = modulo(modulo(seed, M) * 2654435761_8 + modulo(k, M) + 1_8, M)
    x = modulo(ieor(x, ishft(x, -16)) * 1812433253_8, M)
    x = modulo(ieor(x, ishft(x, -13)) * 1664525_8,    M)
    x = modulo(ieor(x, ishft(x, -16)), M)
    u = real(x, 8) / real(M, 8)
  end function draw_rand01

  ! 行列ベクトル積 y = A p。A は n×n 格子の 2次元ラプラシアン (5点ステンシル,
  ! ディリクレ境界=0)。これは B1 (2D熱伝導) や G1 (CG) と同じラプラシアン行列。
  ! 添字は 0..n*n-1 (idx = i*n + j)。
  subroutine matvec(n, p, y)
    integer, intent(in) :: n
    real(8), intent(in) :: p(0:n*n-1)
    real(8), intent(out) :: y(0:n*n-1)
    integer :: i, j
    real(8) :: v
    ! TODO: 格子点の二重ループを !$omp parallel do collapse(2) private(v) で並列化せよ.
    do i = 0, n - 1
       do j = 0, n - 1
          v = 4.0d0 * p(i*n+j)
          if (i > 0)     v = v - p((i-1)*n+j)
          if (i < n - 1) v = v - p((i+1)*n+j)
          if (j > 0)     v = v - p(i*n+j-1)
          if (j < n - 1) v = v - p(i*n+j+1)
          y(i*n+j) = v
       end do
    end do
    ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
  end subroutine matvec

  ! 内積 a・b
  function dot(N, a, b) result(s)
    integer(8), intent(in) :: N
    real(8), intent(in) :: a(0:N-1), b(0:N-1)
    real(8) :: s
    integer(8) :: k
    s = 0.0d0
    ! TODO: 内積の和を !$omp parallel do reduction(+:s) で並列化せよ.
    do k = 0, N - 1
       s = s + a(k) * b(k)
    end do
    ! TODO: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
  end function dot
end module vibration_mod

program vibration
  use vibration_mod
  use omp_lib
  character(len=32) :: arg
  integer :: n, maxit, it, i, j
  real(8) :: tol, sigma, lamB, lamB_prev, nrm, nrm0, lambda_min, analytic, rel_err
  real(8) :: vn, sgn, vecerr, d, v, pi
  integer(8) :: N8, k
  real(8), allocatable :: x(:), y(:), Ax(:), ve(:)
  pi = 4.0d0 * atan(1.0d0)
  n = 128; tol = 1d-9; maxit = 100000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) maxit
  end if
  N8 = int(n, 8) * n
  sigma = 8.0d0                        ! シフト量 (> lambda_max ≈ 8)

  allocate(x(0:N8-1), y(0:N8-1), Ax(0:N8-1), ve(0:N8-1))

  ! 初期ベクトル: すべて 1 から始めて正規化
  do k = 0, N8 - 1
     x(k) = 1.0d0
  end do
  nrm0 = sqrt(dot(N8, x, x))
  do k = 0, N8 - 1
     x(k) = x(k) / nrm0
  end do

  ! べき乗法: B = sigma*I - A を繰り返し掛ける。
  ! B の最大固有値 = sigma - lambda_min(A) に収束し, 固有ベクトルは基本振動モード。
  lamB = 0.0d0; lamB_prev = 0.0d0
  t0_block: block
    real(8) :: t0, elapsed
    t0 = omp_get_wtime()
    do it = 1, maxit
       call matvec(n, x, Ax)
       do k = 0, N8 - 1
          y(k) = sigma * x(k) - Ax(k)     ! y = B x (逐次のまま)
       end do
       lamB = dot(N8, x, y) / dot(N8, x, x)   ! レイリー商
       nrm = sqrt(dot(N8, y, y))
       do k = 0, N8 - 1
          x(k) = y(k) / nrm                ! 正規化 (逐次のまま)
       end do
       if (it > 1 .and. abs(lamB - lamB_prev) < tol) exit
       lamB_prev = lamB
    end do
    elapsed = omp_get_wtime() - t0

    lambda_min = sigma - lamB
    analytic   = 4.0d0 - 4.0d0 * cos(pi / (n + 1))
    rel_err    = abs(lambda_min - analytic) / analytic

    ! 固有ベクトルの検算: 解析解 v[i,j] = sin(pi(i+1)/(n+1)) sin(pi(j+1)/(n+1))。
    vn = 0.0d0
    do i = 0, n - 1
       do j = 0, n - 1
          v = sin(pi * (i+1) / (n+1)) * sin(pi * (j+1) / (n+1))
          ve(i*n+j) = v; vn = vn + v * v
       end do
    end do
    vn = sqrt(vn)
    if (x((n/2)*n + n/2) >= 0.0d0) then
       sgn = 1.0d0
    else
       sgn = -1.0d0
    end if
    vecerr = 0.0d0
    do k = 0, N8 - 1
       d = sgn * x(k) - ve(k) / vn
       vecerr = vecerr + d * d
    end do
    vecerr = sqrt(vecerr)

    print "(a,i0,a,i0,a,f0.10,a,f0.10,a,es9.2)", &
         "n=", n, ", iters=", min(it, maxit), ", lambda_min=", lambda_min, &
         ", analytic=", analytic, ", rel.err=", rel_err
    print "(a,es9.2,a,f0.4,a,f0.4)", &
         "固有ベクトル(基本振動モード) 相対L2誤差=", vecerr, &
         ", 中央値=", sgn * x((n/2)*n + n/2), " vs 隅の値=", sgn * x(0)
    print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
  end block t0_block
end program vibration
