#ifndef	_LOCALE_H
#include <locale/locale.h>

extern __typeof (uselocale) __uselocale;

libc_hidden_proto (setlocale)

/* This has to be changed whenever a new locale is defined.  */
#define __LC_LAST	13

extern struct loaded_l10nfile *_nl_locale_file_list[] attribute_hidden;

/* Locale object for C locale.  */
extern struct __locale_struct _nl_C_locobj attribute_hidden;

/* Now define the internal interfaces.  */
extern struct lconv *__localeconv (void);

/* Fetch the name of the current locale set in the given category.  */
extern const char *__current_locale_name (int category) attribute_hidden;

#endif
