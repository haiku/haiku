#ifndef _WCHAR_H
#include <wcsmbs/wchar.h>

# ifdef _WCHAR_H

extern __typeof (wcscasecmp_l) __wcscasecmp_l;
extern __typeof (wcsncasecmp_l) __wcsncasecmp_l;
extern __typeof (wcscoll_l) __wcscoll_l;
extern __typeof (wcsxfrm_l) __wcsxfrm_l;
extern __typeof (wcstol_l) __wcstol_l;
extern __typeof (wcstoul_l) __wcstoul_l;
extern __typeof (wcstoll_l) __wcstoll_l;
extern __typeof (wcstoull_l) __wcstoull_l;
extern __typeof (wcstod_l) __wcstod_l;
extern __typeof (wcstof_l) __wcstof_l;
extern __typeof (wcstold_l) __wcstold_l;
extern __typeof (wcsftime_l) __wcsftime_l;
libc_hidden_proto (__wcsftime_l)


libc_hidden_proto (__wcstof_internal)
libc_hidden_proto (__wcstod_internal)
libc_hidden_proto (__wcstold_internal)
libc_hidden_proto (__wcstol_internal)
libc_hidden_proto (__wcstoll_internal)
libc_hidden_proto (__wcstoul_internal)
libc_hidden_proto (__wcstoull_internal)

libc_hidden_proto (__wcscasecmp_l)
libc_hidden_proto (__wcsncasecmp_l)

libc_hidden_proto (fputws_unlocked)
libc_hidden_proto (putwc_unlocked)
libc_hidden_proto (putwc)

libc_hidden_proto (vswscanf)

libc_hidden_proto (mbrtowc)
libc_hidden_proto (wcrtomb)
libc_hidden_proto (wcscmp)
libc_hidden_proto (wcsftime)
libc_hidden_proto (wcsspn)
libc_hidden_proto (wcschr)
libc_hidden_proto (wcscoll)
libc_hidden_proto (wcspbrk)

libc_hidden_proto (wmemchr)
libc_hidden_proto (wmemset)

/* Now define the internal interfaces.  */
extern int __wcscasecmp (__const wchar_t *__s1, __const wchar_t *__s2)
     __attribute_pure__;
extern int __wcsncasecmp (__const wchar_t *__s1, __const wchar_t *__s2,
			  size_t __n)
     __attribute_pure__;
extern int __wcscoll (__const wchar_t *__s1, __const wchar_t *__s2);
extern size_t __wcslen (__const wchar_t *__s) __attribute_pure__;
extern size_t __wcsnlen (__const wchar_t *__s, size_t __maxlen)
     __attribute_pure__;
extern wchar_t *__wcscat (wchar_t *dest, const wchar_t *src);
extern wint_t __btowc (int __c);
extern int __mbsinit (__const __mbstate_t *__ps);
extern size_t __mbrtowc (wchar_t *__restrict __pwc,
			 __const char *__restrict __s, size_t __n,
			 __mbstate_t *__restrict __p);
libc_hidden_proto (__mbrtowc)
libc_hidden_proto (__mbrlen)
extern size_t __wcrtomb (char *__restrict __s, wchar_t __wc,
			 __mbstate_t *__restrict __ps);
extern size_t __mbsrtowcs (wchar_t *__restrict __dst,
			   __const char **__restrict __src,
			   size_t __len, __mbstate_t *__restrict __ps);
extern size_t __wcsrtombs (char *__restrict __dst,
			   __const wchar_t **__restrict __src,
			   size_t __len, __mbstate_t *__restrict __ps);
extern size_t __mbsnrtowcs (wchar_t *__restrict __dst,
			    __const char **__restrict __src, size_t __nmc,
			    size_t __len, __mbstate_t *__restrict __ps);
extern size_t __wcsnrtombs (char *__restrict __dst,
			    __const wchar_t **__restrict __src,
			    size_t __nwc, size_t __len,
			    __mbstate_t *__restrict __ps);
extern wchar_t *__wcpcpy (wchar_t *__dest, __const wchar_t *__src);
extern wchar_t *__wcpncpy (wchar_t *__dest, __const wchar_t *__src,
			   size_t __n);
extern wchar_t *__wmemcpy (wchar_t *__s1, __const wchar_t *s2,
			   size_t __n);
extern wchar_t *__wmempcpy (wchar_t *__restrict __s1,
			    __const wchar_t *__restrict __s2,
			    size_t __n);
extern wchar_t *__wmemmove (wchar_t *__s1, __const wchar_t *__s2,
			    size_t __n);
extern wchar_t *__wcschrnul (__const wchar_t *__s, wchar_t __wc)
     __attribute_pure__;

extern int __vfwscanf (__FILE *__restrict __s,
		       __const wchar_t *__restrict __format,
		       __gnuc_va_list __arg)
     /* __attribute__ ((__format__ (__wscanf__, 2, 0)) */;
extern int __vswprintf (wchar_t *__restrict __s, size_t __n,
			__const wchar_t *__restrict __format,
			__gnuc_va_list __arg)
     /* __attribute__ ((__format__ (__wprintf__, 3, 0))) */;
extern int __fwprintf (__FILE *__restrict __s,
		       __const wchar_t *__restrict __format, ...)
     /* __attribute__ ((__format__ (__wprintf__, 3, 0))) */;
extern int __vfwprintf (__FILE *__restrict __s,
			__const wchar_t *__restrict __format,
			__gnuc_va_list __arg)
     /* __attribute__ ((__format__ (__wprintf__, 3, 0))) */;


/* Internal functions.  */
extern size_t __mbsrtowcs_l (wchar_t *dst, const char **src, size_t len,
			     mbstate_t *ps, __locale_t l) attribute_hidden;
# endif
#endif
