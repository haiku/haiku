/******************************************************************************
/
/	File:			TrackerAddOn.h
/
/	Description:	Protocol for the process_refs() hook function.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _TRACKER_ADDON_H
#define _TRACKER_ADDON_H

#include <BeBuild.h>

struct	entry_ref;
class	BMessage;

/*-------------------------------------------------------------*/
/*------- This is why you're here: ----------------------------*/
extern "C" _EXPORT void process_refs(entry_ref dir_ref, BMessage* msg, void*);


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _TRACKER_ADDON_H */
