#include <time.h>
#include <features.h>

#if __GNUC__ < 4
#define restrict
#endif

hidden int __month_to_secs(int, int);
hidden long long __year_to_secs(long long, int *);
hidden long long __tm_to_secs(const struct tm *);
hidden int __secs_to_tm(long long, struct tm *);
hidden size_t __strftime_l(char *restrict s, size_t n, const char *restrict f, const struct tm *restrict tm, locale_t loc);
hidden const char *__strftime_fmt_1(char (*)[100], size_t *, int, const struct tm *, locale_t, int);

#include "time_impl_haiku.h"
