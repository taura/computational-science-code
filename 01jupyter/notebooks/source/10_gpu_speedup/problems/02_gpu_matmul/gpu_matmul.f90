program gpu_matmul
  use omp_lib
  character(len=32) :: arg
  integer(8) :: n, i, j, k, err
  real(8), allocatable :: A(:,:), B(:,:), C(:,:)
  real(8) :: s, t0, t1, dt, flops
  n = 1024
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) n
  end if
  allocate(A(n,n), B(n,n), C(n,n))
  A = 1.0d0; B = 1.0d0; C = 0.0d0

  t0 = omp_get_wtime()
  ! 行列積 C = A * B (完成済み).
  ! OMP_TARGET_OFFLOAD=DISABLED ならホスト(CPU)で, MANDATORY ならGPUで実行される.
  !$omp target teams distribute parallel do map(to: A, B) map(from: C) private(j, k, s)
  do i = 1, n
     do j = 1, n
        s = 0.0d0
        do k = 1, n
           s = s + A(i,k) * B(k,j)
        end do
        C(i,j) = s
     end do
  end do
  !$omp end target teams distribute parallel do
  t1 = omp_get_wtime()
  dt = t1 - t0

  ! 検算: A,B が全て 1 なので C(i,j) = n になるはず
  err = 0
  do j = 1, n
     do i = 1, n
        if (C(i,j) /= dble(n)) err = err + 1
     end do
  end do

  flops = 2.0d0 * dble(n) * dble(n) * dble(n)
  if (err == 0) then
     print "(a,i0,a,f0.3,a,f0.3,a)", "n = ", n, ", elapsed = ", dt, " sec, ", flops / dt * 1.0d-9, " GFLOPS, OK"
  else
     print "(a,i0,a,f0.3,a,f0.3,a)", "n = ", n, ", elapsed = ", dt, " sec, ", flops / dt * 1.0d-9, " GFLOPS, NG"
  end if
end program gpu_matmul
