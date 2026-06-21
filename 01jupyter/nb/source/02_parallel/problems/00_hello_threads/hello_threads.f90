program hello_threads
  use omp_lib
  ! TODO: 下の print を !$omp parallel ... !$omp end parallel で囲み, 複数のスレッドで実行させよ.
  ! BEGIN ANSWER
  !$omp parallel
  ! END ANSWER
  print "(a,i0,a,i0)", "hello from thread ", omp_get_thread_num(), &
       " of ", omp_get_num_threads()
  ! BEGIN ANSWER
  !$omp end parallel
  ! END ANSWER
end program hello_threads
