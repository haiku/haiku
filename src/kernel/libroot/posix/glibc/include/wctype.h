#ifndef _WCTYPE_H

#include <wctype/wctype.h>

/* Internal interfaces.  */
extern int __iswspace (wint_t __wc);
extern int __iswctype (wint_t __wc, wctype_t __desc);
extern wctype_t __wctype (__const char *__property);
extern wint_t __towctrans (wint_t __wc, wctrans_t __desc);


#endif
