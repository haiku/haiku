#ifndef _WCHAR_H
#include <wcsmbs/wchar.h>

# ifdef _WCHAR_H
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

# endif
#endif
