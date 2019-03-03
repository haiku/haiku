#include <fenv.h>

int
feholdexcept (fenv_t *e)
{
  fenv_t etmp;
  __fenv_stfsr(etmp);
  *(e) = etmp;
  etmp = etmp & ~((0x1f << 23) | FE_ALL_EXCEPT);
  __fenv_ldfsr(etmp);

  return 0;
}

