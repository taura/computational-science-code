subroutine add(x, y, z)
  real(8) :: x(8), y(8), z(8)
  integer :: i
  do i = 1, 8
     z(i) = x(i) + y(i)
  end do
end subroutine add
