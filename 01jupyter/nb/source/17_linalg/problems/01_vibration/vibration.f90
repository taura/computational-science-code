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
  ! ディリクレ境界=0)。ベクトルは n×n のネイティブ2次元配列で表す。
  subroutine matvec(p, y)
    real(8), intent(in)  :: p(:,:)
    real(8), intent(out) :: y(:,:)
    integer :: i, j, n
    real(8) :: v
    n = size(p, 1)
    ! TODO: 格子点の二重ループを並列化する。
    ! BEGIN ANSWER
    !$omp parallel do collapse(2) private(v)
    ! END ANSWER
    do j = 1, n
       do i = 1, n
          v = 4.0d0 * p(i,j)
          if (i > 1) v = v - p(i-1,j)
          if (i < n) v = v - p(i+1,j)
          if (j > 1) v = v - p(i,j-1)
          if (j < n) v = v - p(i,j+1)
          y(i,j) = v
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end subroutine matvec

  ! 内積 a・b (ベクトルとみなして全要素の積和)
  function dot(a, b) result(s)
    real(8), intent(in) :: a(:,:), b(:,:)
    real(8) :: s
    integer :: i, j
    s = 0.0d0
    ! TODO: 内積の和を並列化する (reduction)。
    ! BEGIN ANSWER
    !$omp parallel do collapse(2) reduction(+:s)
    ! END ANSWER
    do j = 1, size(a, 2)
       do i = 1, size(a, 1)
          s = s + a(i,j) * b(i,j)
       end do
    end do
    ! BEGIN ANSWER
    !$omp end parallel do
    ! END ANSWER
  end function dot
end module vibration_mod

program vibration
  use vibration_mod
  use omp_lib
  character(len=32) :: arg
  integer :: n, maxit, it, i, j
  real(8) :: tol, sigma, lamB, lamB_prev, nrm, nrm0, lambda_min, analytic, rel_err
  real(8) :: vn, sgn, vecerr, v, pi, t0, elapsed
  real(8), allocatable :: x(:,:), y(:,:), Ax(:,:), ve(:,:)
  pi = 4.0d0 * atan(1.0d0)
  n = 208; tol = 1d-9; maxit = 100000
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  if (command_argument_count() >= 3) then
     call get_command_argument(3, arg); read (arg, *) maxit
  end if
  sigma = 8.0d0                        ! シフト量 (> lambda_max ≈ 8)

  allocate(x(n,n), y(n,n), Ax(n,n), ve(n,n))

  ! 初期ベクトル: すべて 1 から始めて正規化
  x = 1.0d0
  nrm0 = sqrt(dot(x, x))
  x = x / nrm0

  ! べき乗法: B = sigma*I - A を繰り返し掛ける。
  ! B の最大固有値 = sigma - lambda_min(A) に収束し, 固有ベクトルは基本振動モード。
  lamB = 0.0d0; lamB_prev = 0.0d0
  t0 = omp_get_wtime()
  do it = 1, maxit
     call matvec(x, Ax)
     y = sigma * x - Ax                 ! y = B x (逐次のまま)
     lamB = dot(x, y) / dot(x, x)        ! レイリー商
     nrm = sqrt(dot(y, y))
     x = y / nrm                         ! 正規化 (逐次のまま)
     if (it > 1 .and. abs(lamB - lamB_prev) < tol) exit
     lamB_prev = lamB
  end do
  elapsed = omp_get_wtime() - t0

  lambda_min = sigma - lamB
  analytic   = 4.0d0 - 4.0d0 * cos(pi / (n + 1))
  rel_err    = abs(lambda_min - analytic) / analytic

  ! 固有ベクトルの検算: 解析解 v(i,j) = sin(pi i/(n+1)) sin(pi j/(n+1)) (i,j は 1..n)。
  do j = 1, n
     do i = 1, n
        ve(i,j) = sin(pi * i / (n+1)) * sin(pi * j / (n+1))
     end do
  end do
  vn = sqrt(sum(ve * ve))
  if (x(n/2+1, n/2+1) >= 0.0d0) then
     sgn = 1.0d0
  else
     sgn = -1.0d0
  end if
  vecerr = sqrt(sum((sgn * x - ve / vn)**2))

  print "(a,i0,a,i0,a,f0.10,a,f0.10,a,es9.2)", &
       "n=", n, ", iters=", min(it, maxit), ", lambda_min=", lambda_min, &
       ", analytic=", analytic, ", rel.err=", rel_err
  print "(a,es9.2,a,f0.4,a,f0.4)", &
       "固有ベクトル(基本振動モード) 相対L2誤差=", vecerr, &
       ", 中央値=", sgn * x(n/2+1, n/2+1), " vs 隅の値=", sgn * x(1, 1)
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program vibration
