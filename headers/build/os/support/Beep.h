/******************************************************************************
/
/	File:			Beep.h
/
/	Description:	System beep function.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _BEEP_H
#define _BEEP_H

#include <BeBuild.h>
#include <SupportDefs.h>


_IMPEXP_BE status_t beep();
_IMPEXP_BE status_t system_beep(const char * event_name);

#if defined(__cplusplus)
#define _BEEP_FLAGS = 0
#else
#define _BEEP_FLAGS
#endif
_IMPEXP_BE status_t add_system_beep_event(const char * event_name, uint32 flags _BEEP_FLAGS);


#endif	/* _BEEP_H */
