/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHECKITOUT_H
#define _CHECKITOUT_H
 
#include <Application.h>
#include <String.h>


class CheckItOut : public BApplication
{
public:
								CheckItOut();
								~CheckItOut();

	virtual void				RefsReceived(BMessage* msg);
	virtual void				MessageReceived(BMessage* msg);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun(void);

private:
			status_t			_Warn(const char* url);
			status_t			_DecodeUrlString(BString& string);
			status_t			_FilePanel(uint32 nodeFlavors, BString &name);
			status_t			_DoCheckItOut(entry_ref *ref, const char *name);

	BString						fUrlString;
};

#endif // _CHECKITOUT_H

