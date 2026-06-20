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
  ! これは B1 (2D熱伝導) と同じラプラシアン行列。添字は 0..n*n-1 (idx = i*n + j)。
  subroutine matvec(n, p, y)
    integer, intent(in) :: n
    real(8), intent(in) :: p(0:n*n-1)
    real(8), intent(out) :: y(0:n*n-1)
    integer :: i, j
    real(8) :: v
    ! BEGIN ANSWER: 格子点の二重ループを !$omp parallel do collapse(2) private(v) で並列化せよ.
    !$omp parallel do collapse(2) private(v)
    ! END ANSWER
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
    ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
    !$omp end parallel do
    ! END ANSWER
  end subroutine matvec

  ! 内積 a・b
  function dot(N, a, b) result(s)
    integer(8), intent(in) :: N
    real(8), intent(in) :: a(0:N-1), b(0:N-1)
    real(8) :: s
    integer(8) :: k
    s = 0.0d0
    ! BEGIN ANSWER: 内積の和を !$omp parallel do reduction(+:s) で並列化せよ.
    !$omp parallel do reduction(+:s)
    ! END ANSWER
    do k = 0, N - 1
       s = s + a(k) * b(k)
    end do
    ! BEGIN ANSWER: 上で始めた parallel do 領域を閉じる (!$omp end parallel do)。
    !$omp end parallel do
    ! END ANSWER
  end function dot
end module cg_mod

program cg
  use cg_mod
  use omp_lib
  character(len=32) :: arg
  integer :: n, maxit, it
  real(8) :: tol, rs, rs_new, alpha, beta, err, e, t0, elapsed
  integer(8) :: N8, k
  real(8), allocatable :: xt(:), b(:), x(:), r(:), p(:), Ap(:)
  n = 128; tol = 1d-8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  if (command_argument_count() >= 2) then
     call get_command_argument(2, arg); read (arg, *) tol
  end if
  N8 = int(n, 8) * n
  maxit = 10 * n

  allocate(xt(0:N8-1), b(0:N8-1), x(0:N8-1), r(0:N8-1), p(0:N8-1), Ap(0:N8-1))
  do k = 0, N8 - 1
     xt(k) = draw_rand01(k, 0_8)        ! 真の解をランダムに決め
  end do
  call matvec(n, xt, b)                 ! b = A xt を作る

  ! CG: x=0 から始めて A x = b を解く
  do k = 0, N8 - 1
     x(k) = 0.0d0; r(k) = b(k); p(k) = b(k)
  end do
  rs = dot(N8, r, r)

  t0 = omp_get_wtime()
  do it = 1, maxit
     call matvec(n, p, Ap)
     alpha = rs / dot(N8, p, Ap)
     do k = 0, N8 - 1
        x(k) = x(k) + alpha * p(k); r(k) = r(k) - alpha * Ap(k)   ! (発展: ここも並列化可)
     end do
     rs_new = dot(N8, r, r)
     if (sqrt(rs_new) < tol) then
        rs = rs_new; exit
     end if
     beta = rs_new / rs
     do k = 0, N8 - 1
        p(k) = r(k) + beta * p(k)
     end do
     rs = rs_new
  end do
  elapsed = omp_get_wtime() - t0

  ! 検算: 求めた x が真の解 xt にどれだけ近いか
  err = 0.0d0
  do k = 0, N8 - 1
     e = abs(x(k) - xt(k)); if (e > err) err = e
  end do
  print "(a,i0,a,i0,a,i0,a,es9.2,a,es9.2)", &
       "n=", n, " (N=", N8, "), iters=", min(it, maxit), &
       ", 残差=", sqrt(rs), ", 解の誤差(max|x-xt|)=", err
  print "(a,f0.3,a)", "elapsed = ", elapsed, " sec"
end program cg
