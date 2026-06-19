! C = A * B (いずれも n x n 行列). i-k-j 順のループ.
program matmul
  use omp_lib
  implicit none
  character(len=32) :: arg
  integer(8) :: n, i, j, k, err
  real(8), allocatable :: A(:,:), B(:,:), C(:,:)
  real(8) :: a_ik, t0, dt, gflops, expected
  n = 1024
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(A(n,n), B(n,n), C(n,n))
  A = 1.0d0; B = 2.0d0; C = 0.0d0

  t0 = omp_get_wtime()
  !$omp parallel do private(j,k,a_ik)
  do i = 1, n
     do k = 1, n
        a_ik = A(i,k)
        ! BEGIN ANSWER: 最内 j ループを omp simd でSIMD化せよ (下の do の直前に1行追加).
        !$omp simd
        ! END ANSWER
        do j = 1, n
           C(i,j) = C(i,j) + a_ik * B(k,j)
        end do
     end do
  end do
  dt = omp_get_wtime() - t0

  gflops = 2.0d0 * real(n, 8) * real(n, 8) * real(n, 8) / dt * 1e-9
  ! A=1, B=2 なので C の各要素は 2*n になるはず.
  expected = 2.0d0 * real(n, 8)
  err = 0
  do j = 1, n
     do i = 1, n
        if (C(i,j) /= expected) err = err + 1
     end do
  end do
  if (err == 0) then
     print "(a,i0,a,f0.3,a)", "n=", n, " : ", gflops, " GFLOPS  (check: OK)"
  else
     print "(a,i0,a,f0.3,a)", "n=", n, " : ", gflops, " GFLOPS  (check: NG)"
  end if
end program matmul
