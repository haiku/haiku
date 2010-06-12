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
#include <string.h>

#include "constants.h"
#include "stddlg.h"

void ierror(const char* msg)
{
	char* fullMsg = new char[strlen(STR_IERROR) + strlen(msg) + 1];
	strcpy(fullMsg, STR_IERROR);
	strcpy(fullMsg, msg);
	BAlert alert("Internal Error", fullMsg, "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert.Go();
	delete [] fullMsg;
}
