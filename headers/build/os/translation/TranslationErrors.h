/********************************************************************************
/
/      File:           TranslationErrors.h
/
/      Description:    Error codes for the Translation Kit
/
/      Copyright 1998, Be Incorporated, All Rights Reserved.
/      Copyright 1995-1997, Jon Watte
/
********************************************************************************/

#if !defined(_TRANSLATION_ERRORS_H)
#define _TRANSLATION_ERRORS_H

#include <Errors.h>


enum B_TRANSLATION_ERROR {
	B_TRANSLATION_BASE_ERROR = B_TRANSLATION_ERROR_BASE,
	B_NO_TRANSLATOR = B_TRANSLATION_BASE_ERROR,	/*	no translator exists for data	*/
	B_ILLEGAL_DATA,	/*	data is not what it said it was	*/
/* aliases */
	B_NOT_INITIALIZED = B_NO_INIT
};


#endif /* _TRANSLATION_ERRORS_H */
