//--------------------------------------------------------------------
//	
//	main.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PeopleApp.h"

int main(void)
{	
	TPeopleApp	*app;

	app = new TPeopleApp();
	app->Run();

	delete app;
	return B_NO_ERROR;
}
