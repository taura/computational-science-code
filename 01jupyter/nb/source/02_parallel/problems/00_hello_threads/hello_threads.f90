program hello_threads
  use omp_lib
  ! BEGIN ANSWER: 下の print を !$omp parallel ... !$omp end parallel で囲み, 複数のスレッドで実行させよ.
  !$omp parallel
  ! END ANSWER
  print "(a,i0,a,i0)", "hello from thread ", omp_get_thread_num(), &
       " of ", omp_get_num_threads()
  ! BEGIN ANSWER: 上で始めた parallel 領域を閉じる (!$omp end parallel).
  !$omp end parallel
  ! END ANSWER
end program hello_threads
