program hello_threads
  use omp_lib
  ! TODO: 下の print を !$omp parallel ... !$omp end parallel で囲み, 複数のスレッドで実行させよ.
  print "(a,i0,a,i0)", "hello from thread ", omp_get_thread_num(), &
       " of ", omp_get_num_threads()
  ! TODO: 上で始めた parallel 領域を閉じる (!$omp end parallel).
end program hello_threads
