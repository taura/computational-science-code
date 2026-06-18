program omp_parallel_rank
  use omp_lib
  integer :: omp_nthreads, omp_rank
  print "(a)", "hello"
  !$omp parallel private(omp_nthreads, omp_rank)
  omp_nthreads = omp_get_num_threads()
  omp_rank = omp_get_thread_num()
  print "(a,i3,a,i3)", "world ", omp_rank, "/", omp_nthreads
  !$omp end parallel
  print "(a)", "good bye"
end program omp_parallel_rank
