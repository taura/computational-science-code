module cg_mod
contains
  ! 状態を持たない乱数 (既知解の生成用): (seed,k) から [0,1)。
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
  ! ディリクレ境界=0) で対称正定値。行列を保持せずステンシルで計算する (行列フリー)。
  ! ベクトルは n×n のネイティブ2次元配列で表す。
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
end module cg_mod

program cg
  use cg_mod
  use omp_lib
  character(len=32) :: arg
  integer :: n, maxit, it, i, j
  real(8) :: tol, rs, rs_new, alpha, beta, err, t0, elapsed
  real(8), allocatable :: xt(:,:), b(:,:), x(:,:), r(:,:), p(:,:), Ap(:,:)
  n = 128; tol = 1d-8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  maxit = 10 * n

  allocate(xt(n,n), b(n,n), x(n,n), r(n,n), p(n,n), Ap(n,n))
  do i = 1, n
     do j = 1, n
        xt(i,j) = draw_rand01(int((i-1)*n + (j-1), 8), 0_8)   ! 真の解をランダムに決め
     end do
  end do
  call matvec(xt, b)                    ! b = A xt を作る

  ! CG: x=0 から始めて A x = b を解く
  x = 0.0d0; r = b; p = b
  rs = dot(r, r)

  t0 = omp_get_wtime()
  do it = 1, maxit
     call matvec(p, Ap)
     alpha = rs / dot(p, Ap)
     x = x + alpha * p; r = r - alpha * Ap     ! (発展: ここも並列化可)
     rs_new = dot(r, r)
     if (sqrt(rs_new) < tol) then
        rs = rs_new; exit
     end if
     beta = rs_new / rs
     p = r + beta * p
     rs = rs_new
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算: 求めた x が真の解 xt にどれだけ近いか
  err = maxval(abs(x - xt))
  print "(a,i0,a,i0,a,i0,a,es9.2,a,es9.2)", &
       "n=", n, " (N=", n*n, "), iters=", min(it, maxit), &
       ", 残差=", sqrt(rs), ", 解の誤差(max|x-xt|)=", err
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program cg
