#ifndef _STDLIB_H

#ifdef __need_malloc_and_calloc
#define __Need_M_And_C
#endif

#include <stddef.h>
#include <stdlib/stdlib.h>

/* Now define the internal interfaces.  */
#ifndef __Need_M_And_C

extern __typeof (strtol_l) __strtol_l;
extern __typeof (strtoul_l) __strtoul_l;
extern __typeof (strtoll_l) __strtoll_l;
extern __typeof (strtoull_l) __strtoull_l;
extern __typeof (strtod_l) __strtod_l;
extern __typeof (strtof_l) __strtof_l;
extern __typeof (strtold_l) __strtold_l;

libc_hidden_proto (exit)
libc_hidden_proto (abort)
libc_hidden_proto (getenv)
libc_hidden_proto (bsearch)
libc_hidden_proto (qsort)
libc_hidden_proto (ecvt_r)
libc_hidden_proto (fcvt_r)
libc_hidden_proto (qecvt_r)
libc_hidden_proto (qfcvt_r)
libc_hidden_proto (lrand48_r)
libc_hidden_proto (wctomb)
libc_hidden_proto (__secure_getenv)
libc_hidden_proto (__strtof_internal)
libc_hidden_proto (__strtod_internal)
libc_hidden_proto (__strtold_internal)
libc_hidden_proto (__strtol_internal)
libc_hidden_proto (__strtoll_internal)
libc_hidden_proto (__strtoul_internal)
libc_hidden_proto (__strtoull_internal)

extern long int __random (void);
extern void __srandom (unsigned int __seed);
extern char *__initstate (unsigned int __seed, char *__statebuf,
			  size_t __statelen);
extern char *__setstate (char *__statebuf);
extern int __random_r (struct random_data *__buf, int32_t *__result);
extern int __srandom_r (unsigned int __seed, struct random_data *__buf);
extern int __initstate_r (unsigned int __seed, char *__statebuf,
			  size_t __statelen, struct random_data *__buf);
extern int __setstate_r (char *__statebuf, struct random_data *__buf);
extern int __rand_r (unsigned int *__seed);
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

/* Global state for non-reentrant functions.  Defined in drand48-iter.c.  */
extern struct drand48_data __libc_drand48_data attribute_hidden;

extern int __setenv (__const char *__name, __const char *__value,
		     int __replace);
extern int __unsetenv (__const char *__name);
extern int __clearenv (void);
extern char *__canonicalize_file_name (__const char *__name);
extern char *__realpath (__const char *__name, char *__resolved);
extern int __ptsname_r (int __fd, char *__buf, size_t __buflen);
extern int __getpt (void);
extern int __posix_openpt (int __oflag);

extern int __add_to_environ (const char *name, const char *value,
			     const char *combines, int replace);

extern void _quicksort (void *const pbase, size_t total_elems,
			size_t size, __compar_fn_t cmp);

extern int __on_exit (void (*__func) (int __status, void *__arg), void *__arg);

extern int __cxa_atexit (void (*func) (void *), void *arg, void *d);
extern int __cxa_atexit_internal (void (*func) (void *), void *arg, void *d)
     attribute_hidden;

extern void __cxa_finalize (void *d);

extern int __posix_memalign (void **memptr, size_t alignment, size_t size)
     __attribute_malloc__;
extern void *__libc_memalign (size_t alignment, size_t size)
     __attribute_malloc__;

extern int __libc_system (const char *line);

extern double ____strtod_l_internal (__const char *__restrict __nptr,
				     char **__restrict __endptr, int __group,
				     __locale_t __loc) __THROW;
extern float ____strtof_l_internal (__const char *__restrict __nptr,
				    char **__restrict __endptr, int __group,
				    __locale_t __loc) __THROW;
extern long double ____strtold_l_internal (__const char *__restrict __nptr,
					   char **__restrict __endptr,
					   int __group, __locale_t __loc)
     __THROW;
extern long int ____strtol_l_internal (__const char *__restrict __nptr,
				       char **__restrict __endptr,
				       int __base, int __group,
				       __locale_t __loc) __THROW;
extern unsigned long int ____strtoul_l_internal (__const char *
						 __restrict __nptr,
						 char **__restrict __endptr,
						 int __base, int __group,
						 __locale_t __loc) __THROW;
__extension__
extern long long int ____strtoll_l_internal (__const char *__restrict __nptr,
					     char **__restrict __endptr,
					     int __base, int __group,
					     __locale_t __loc) __THROW;
__extension__
extern unsigned long long int ____strtoull_l_internal (__const char *
						       __restrict __nptr,
						       char **
						       __restrict __endptr,
						       int __base, int __group,
						       __locale_t __loc)
     __THROW;

libc_hidden_proto (____strtof_l_internal)
libc_hidden_proto (____strtod_l_internal)
libc_hidden_proto (____strtold_l_internal)
libc_hidden_proto (____strtol_l_internal)
libc_hidden_proto (____strtoll_l_internal)
libc_hidden_proto (____strtoul_l_internal)
libc_hidden_proto (____strtoull_l_internal)

extern __inline double
__strtod_l (__const char *__restrict __nptr, char **__restrict __endptr,
	    __locale_t __loc) __THROW
{
  return ____strtod_l_internal (__nptr, __endptr, 0, __loc);
}
extern __inline long int
__strtol_l (__const char *__restrict __nptr, char **__restrict __endptr,
	    int __base, __locale_t __loc) __THROW
{
  return ____strtol_l_internal (__nptr, __endptr, __base, 0, __loc);
}
extern __inline unsigned long int
__strtoul_l (__const char *__restrict __nptr, char **__restrict __endptr,
	     int __base, __locale_t __loc) __THROW
{
  return ____strtoul_l_internal (__nptr, __endptr, __base, 0, __loc);
}
extern __inline float
__strtof_l (__const char *__restrict __nptr, char **__restrict __endptr,
	    __locale_t __loc) __THROW
{
  return ____strtof_l_internal (__nptr, __endptr, 0, __loc);
}
extern __inline long double
__strtold_l (__const char *__restrict __nptr, char **__restrict __endptr,
	     __locale_t __loc) __THROW
{
  return ____strtold_l_internal (__nptr, __endptr, 0, __loc);
}
__extension__ extern __inline long long int
__strtoll_l (__const char *__restrict __nptr, char **__restrict __endptr,
	     int __base, __locale_t __loc) __THROW
{
  return ____strtoll_l_internal (__nptr, __endptr, __base, 0, __loc);
}
__extension__ extern __inline unsigned long long int
__strtoull_l (__const char * __restrict __nptr, char **__restrict __endptr,
	      int __base, __locale_t __loc) __THROW
{
  return ____strtoull_l_internal (__nptr, __endptr, __base, 0, __loc);
}


# ifndef NOT_IN_libc
#  undef MB_CUR_MAX
#  define MB_CUR_MAX (_NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_MB_CUR_MAX))

# define __cxa_atexit(func, arg, d) INTUSE(__cxa_atexit) (func, arg, d)
# endif

#endif

extern void * __default_morecore (ptrdiff_t);
libc_hidden_proto (__default_morecore)

#undef __Need_M_And_C

#endif  /* include/stdlib.h */
