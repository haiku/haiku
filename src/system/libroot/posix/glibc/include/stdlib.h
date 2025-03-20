#ifndef _GLIBC_STDLIB_H
#define _GLIBC_STDLIB_H

#include_next <stdlib.h>
#include <stdint.h>

#include <sys/cdefs.h>


/* Now define the internal interfaces.  */
extern long int __random (void);
extern void __srandom (unsigned int __seed);
extern char *__initstate (unsigned int __seed, char *__statebuf,
			  size_t __statelen);
extern char *__setstate (char *__statebuf);
extern int __random_r (struct random_data *__buf, int32_t *__result);
extern int __srandom_r (unsigned int __seed, struct random_data *__buf);
extern int __rand_r (unsigned int *__seed);

struct drand48_data {
	unsigned short int __x[3];
	unsigned short int __old_x[3];
	unsigned short int __c;
	unsigned short int __init;
	unsigned long long int __a;
};
extern int __erand48_r (unsigned short int __xsubi[3],
			struct drand48_data *__buffer, double *__result);
extern int __nrand48_r (unsigned short int __xsubi[3],
			struct drand48_data *__buffer,
			long int *__result);
extern int __jrand48_r (unsigned short int __xsubi[3],
			struct drand48_data *__buffer,
			long int *__result);
extern int __srand48_r (long int __seedval,
			struct drand48_data *__buffer);
extern int __seed48_r (unsigned short int __seed16v[3],
		       struct drand48_data *__buffer);
extern int __lcong48_r (unsigned short int __param[7],
			struct drand48_data *__buffer);

/* Internal function to compute next state of the generator.  */
extern int __drand48_iterate (unsigned short int __xsubi[3],
			      struct drand48_data *__buffer);

extern int __cxa_atexit (void (*func) (void *), void *arg, void *d);

extern void __cxa_finalize (void *d);

/* The internal entry points for `strtoX' take an extra flag argument
   saying whether or not to parse locale-dependent number grouping.  */

extern double __strtod_internal (__const char *__restrict __nptr,
				 char **__restrict __endptr, int __group)
	 __THROW;
extern float __strtof_internal (__const char *__restrict __nptr,
				char **__restrict __endptr, int __group)
	 __THROW;
extern long double __strtold_internal (__const char *__restrict __nptr,
					   char **__restrict __endptr,
					   int __group) __THROW;
#ifndef __strtol_internal_defined
extern long int __strtol_internal (__const char *__restrict __nptr,
				   char **__restrict __endptr,
				   int __base, int __group) __THROW;
# define __strtol_internal_defined	1
#endif
#ifndef __strtoul_internal_defined
extern unsigned long int __strtoul_internal (__const char *__restrict __nptr,
						 char **__restrict __endptr,
						 int __base, int __group) __THROW;
# define __strtoul_internal_defined	1
#endif
#if defined __GNUC__ || defined __USE_ISOC99
# ifndef __strtoll_internal_defined
__extension__
extern long long int __strtoll_internal (__const char *__restrict __nptr,
					 char **__restrict __endptr,
					 int __base, int __group) __THROW;
#  define __strtoll_internal_defined	1
# endif
# ifndef __strtoull_internal_defined
__extension__
extern unsigned long long int __strtoull_internal (__const char *
						   __restrict __nptr,
						   char **__restrict __endptr,
						   int __base, int __group)
	 __THROW;
#  define __strtoull_internal_defined	1
# endif
#endif /* GCC */

#endif  /* include/stdlib.h */
