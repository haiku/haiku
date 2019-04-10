#ifndef _MATH_H
# error "Never use <bits/huge_val.h> directly; include <math.h> instead."
#endif


# define HUGE_VAL	(__builtin_huge_val())
# define HUGE_VALF  (__builtin_huge_valf())
# define HUGE_VALL  (__builtin_huge_vall())
