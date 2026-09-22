/* PatchApp.h
 * ----------
 * The PatchBay application class.
 *
 * Copyright 2013-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by:
 * 		Pete Goodeve
 *		Philippe Houdoin
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PATCHAPP_H
#define PATCHAPP_H

#include <Application.h>

extern const char * kApplicationName;
extern const char * kApplicationSignature;


class PatchApp : public BApplication
{
public:
	PatchApp();
	void ReadyToRun();
	void MessageReceived(BMessage* msg);
};

#endif /* PATCHAPP_H */
