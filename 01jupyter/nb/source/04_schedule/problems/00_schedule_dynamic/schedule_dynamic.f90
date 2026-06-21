module work_mod
contains
  ! 仕事量が引数 k に比例するダミー計算 (k に比例した回数だけ加算する)
  function work(k) result(s)
    integer(8), intent(in) :: k
    real(8) :: s
    integer(8) :: j
    s = 0.0d0
    do j = 0, k - 1
       s = s + 1.0d0 / (1.0d0 + j)
    end do
  end function work
end module work_mod

program schedule_dynamic
  use omp_lib
  use work_mod
  integer, parameter :: n = 2000
  integer :: i
  real(8) :: total
  total = 0.0d0
  ! TODO: 下のループを並列化し, 仕事量が i に比例して不均一なので schedule(dynamic) で負荷を均せ.
  ! BEGIN ANSWER
  !$omp parallel do schedule(dynamic) reduction(+:total)
  ! END ANSWER
  do i = 0, n - 1
     ! 繰り返し i の仕事量は i に比例して重くなる (アンバランス)
     total = total + work(int(i, 8) * 100000_8)
  end do
  ! BEGIN ANSWER
  !$omp end parallel do
  ! END ANSWER
  print "(a,f0.6)", "total = ", total
end program schedule_dynamic
