// PatchApp.h
// ----------
// The PatchBay application class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _PatchApp_h
#define _PatchApp_h

#include <Application.h>

class TToolTip;

class PatchApp : public BApplication
{
public:
	PatchApp();
	void ReadyToRun();
	void MessageReceived(BMessage* msg);
	
private:
	TToolTip* m_toolTip;
};

#endif /* _PatchApp_h */
