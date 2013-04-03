/* PatchApp.h
 * ----------
 * The PatchBay application class.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#ifndef _PatchApp_h
#define _PatchApp_h

#include <Application.h>

class PatchApp : public BApplication
{
public:
	PatchApp();
	void ReadyToRun();
	void MessageReceived(BMessage* msg);
};

#endif /* _PatchApp_h */
