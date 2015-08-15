#include <math.h>
#include <stdio.h>
#include <errno.h>

long double
__atanl (long double x)
{
  fputs ("__atanl not implemented\n", stderr);
  __set_errno (ENOSYS);
  return 0.0;
}
weak_alias (__atanl, atanl)

stub_warning (atanl)
