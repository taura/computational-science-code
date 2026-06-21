program matvec
  implicit none
  integer :: n, i, j
  real(8), allocatable :: A(:), x(:), y(:)
  real(8) :: s
  logical :: ok
  character(len=32) :: arg

  ! 行列・ベクトルのサイズ (コマンドライン引数, 既定 4000)
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg)
     read(arg, *) n
  else
     n = 4000
  end if

  allocate(A(0:n*n-1), x(0:n-1), y(0:n-1))

  ! 検算しやすい初期化: A(i*n+j) = 1, x(j) = 1 とすると y(i) = n になる.
  ! (この初期化ループは collapse(2) で並列化してもよい)
  do i = 0, n - 1
     do j = 0, n - 1
        A(i*n + j) = 1.0d0
     end do
  end do
  do j = 0, n - 1
     x(j) = 1.0d0
  end do

  ! 行列ベクトル積 y = A x
  ! TODO: 下の外側の do ループを !$omp parallel do private(j, s) ... !$omp end parallel do で囲み, 行ごとの計算をスレッドで分担せよ.
  ! BEGIN ANSWER
  !$omp parallel do private(j, s)
  ! END ANSWER
  do i = 0, n - 1
     s = 0.0d0  ! 行ごとの局所アキュムレータ (reduction 不要)
     do j = 0, n - 1
        s = s + A(i*n + j) * x(j)
     end do
     y(i) = s
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER

  ! 検算: すべての y(i) が n に等しいはず
  ok = .true.
  do i = 0, n - 1
     if (y(i) /= real(n, 8)) then
        ok = .false.
        exit
     end if
  end do
  if (ok) then
     print "(a,i0,a,f0.6,a,i0,a)", "n = ", n, ", y(0) = ", y(0), &
          " (expected ", n, "): OK"
  else
     print "(a,i0,a,f0.6,a,i0,a)", "n = ", n, ", y(0) = ", y(0), &
          " (expected ", n, "): NG"
  end if

  deallocate(A, x, y)
end program matvec
