program gpu_matvec
  character(len=32) :: arg
  integer(8) :: n, i, j, err
  real(8), allocatable :: A(:,:), x(:), y(:)
  real(8) :: s
  n = 4096
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(A(n,n), x(n), y(n))
  do i = 1, n
     x(i) = 1.0d0
     y(i) = -1.0d0          ! 番兵: 未計算なら検算に失敗する
     do j = 1, n
        A(j,i) = 1.0d0
     end do
  end do

  ! 行列ベクトル積 y = A x を GPU で計算する.
  ! A は n*n 要素, x, y は n 要素. A,x は入力 (to:), y は結果 (from:).
  ! BEGIN ANSWER
  !$omp target teams distribute parallel do map(to: A, x) map(from: y) private(j, s)
  do i = 1, n
     s = 0.0d0
     do j = 1, n
        s = s + A(j,i) * x(j)
     end do
     y(i) = s
  end do
  !$omp end target teams distribute parallel do
  ! END ANSWER

  ! 検算: A(j,i)=1, x(j)=1 なので y(i) = n になるはず
  err = 0
  do i = 1, n
     if (y(i) /= dble(n)) err = err + 1
  end do
  if (err == 0) then
     print "(a,i0,a,f0.0,a)", "OK: n = ", n, ", y(1) = ", y(1), " (= n)"
  else
     print "(a,i0,a)", "NG: ", err, " 要素が不正"
  end if
end program gpu_matvec
