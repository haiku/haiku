#ifndef _B_LOCALE_BUILD_H_
#define _B_LOCALE_BUILD_H_

#if _BUILDING_locale
#define	_IMPEXP_LOCALE	__declspec(dllexport)
#else
#define	_IMPEXP_LOCALE	__declspec(dllimport)
#endif



#endif	/* _B_LOCALE_BUILD_H_ */
