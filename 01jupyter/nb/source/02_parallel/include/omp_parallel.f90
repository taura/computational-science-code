program omp_parallel
  print "(a)", "before parallel"
  !$omp parallel
  print "(a)", "in parallel"
  !$omp end parallel
  print "(a)", "after parallel"
end program omp_parallel
