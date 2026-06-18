program omp_target
  print "(a)", "hello on host"
  !$omp target
  print "(a)", "hello from target (hopefully GPU)"
  !$omp end target
  print "(a)", "back on host"
end program omp_target
