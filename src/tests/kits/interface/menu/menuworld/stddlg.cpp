//--------------------------------------------------------------------
//
//	stddlg.cpp
//
//	Written by: Owen Smith
//
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>
#include <String.h>

#include "constants.h"
#include "stddlg.h"


void ierror(const char* msg)
{
	BString fullMsg(STR_IERROR);
	fullMsg << msg;
	BAlert alert("Internal Error", fullMsg, "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert.Go();
}
