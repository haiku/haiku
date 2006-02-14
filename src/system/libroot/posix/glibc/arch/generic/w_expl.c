#include <math.h>
#include <stdio.h>
#include <errno.h>

long double
__expl(long double x)
{
  fputs ("__expl not implemented\n", stderr);
  __set_errno (ENOSYS);
  return 0.0;
}

weak_alias (__expl, expl)
