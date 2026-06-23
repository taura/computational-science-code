module prime_mod
contains
  function is_prime(k) result(r)
    integer(8), intent(in) :: k
    integer :: r
    integer(8) :: d
    if (k < 2) then
       r = 0
       return
    end if
    d = 2
    do while (d * d <= k)
       if (mod(k, d) == 0) then
          r = 0
          return
       end if
       d = d + 1
    end do
    r = 1
  end function is_prime
end module prime_mod

program count_primes
  use prime_mod
  character(len=64) :: arg
  integer(8) :: N, i, count
  N = 8000000_8
  if (command_argument_count() >= 1) then
     call get_command_argument(1, arg); read (arg, *) N
  end if
  count = 0
  ! TODO: 下のループを !$omp parallel do schedule(runtime) reduction(+:count) で並列化せよ.
  ! BEGIN ANSWER
  !$omp parallel do schedule(runtime) reduction(+:count)
  ! END ANSWER
  do i = 2, N
     count = count + is_prime(i)
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  print "(a,i0,a,i0)", "number of primes <= ", N, " : ", count
end program count_primes
