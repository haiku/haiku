/*
 * Copyright 2005-2011 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WCTYPE_H_
#define _WCTYPE_H_


#include <wchar.h>

typedef int wctrans_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int		iswalnum(wint_t wc);
extern int		iswalpha(wint_t wc);
extern int		iswcntrl(wint_t wc);
extern int		iswctype(wint_t wc, wctype_t desc);
extern int		iswdigit(wint_t wc);
extern int		iswgraph(wint_t wc);
extern int		iswlower(wint_t wc);
extern int		iswprint(wint_t wc);
extern int		iswpunct(wint_t wc);
extern int		iswspace(wint_t wc);
extern int		iswupper(wint_t wc);
extern int		iswxdigit(wint_t wc);

extern int		iswblank(wint_t wc);

extern wint_t	towctrans(wint_t wc, wctrans_t transition);
extern wint_t	towlower(wint_t wc);
extern wint_t	towupper(wint_t wc);

extern wctrans_t wctrans(const char *charClass);
extern wctype_t	wctype(const char *property);

#ifdef __cplusplus
}
#endif

#endif	/* _WCTYPE_H_ */
