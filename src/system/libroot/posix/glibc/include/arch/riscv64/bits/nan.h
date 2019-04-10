#ifndef _MATH_H
# error "Never use <bits/nan.h> directly; include <math.h> instead."
#endif

# define NAN	(__builtin_nanf (""))
