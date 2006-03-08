/*
	NodeHarnessApp.h

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef NodeHarnessApp_H
#define NodeHarnessApp_H 1

#include <app/Application.h>

class NodeHarnessApp : public BApplication
{
public:
	NodeHarnessApp(const char* signature);
	void ReadyToRun();
};

#endif
