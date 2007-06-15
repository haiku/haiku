/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"

int				driverFileDesc;
SharedInfo*		si;
area_id			sharedInfoArea;
uint8*			regs;
area_id			regsArea;
display_mode*	modeList;
area_id			modeListArea;
bool			bAccelerantIsClone;

