module power_mod
contains
  ! x を n 乗して返す関数 (x^n = x を n 回掛けたもの)
  function power(x, n) result(p)
    real(8), intent(in) :: x
    integer, intent(in) :: n
    real(8) :: p
    integer :: i
    p = 1.0d0
    ! TODO: x を n 回掛けて x^n を計算し p に求めよ (ループを書く).
    ! BEGIN ANSWER
    do i = 1, n
       p = p * x
    end do
    ! END ANSWER
  end function power
end module power_mod

program func
  use power_mod
  print "(a,f0.6)", "2^10 = ", power(2.0d0, 10)
  print "(a,f0.6)", "3^4  = ", power(3.0d0, 4)
end program func
