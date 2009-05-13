/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _URL_WRAPPER_H
#define _URL_WRAPPER_H
 
#include <Application.h>
#include <String.h>


class UrlWrapper : public BApplication
{
public:
								UrlWrapper();
								~UrlWrapper();

	virtual void				RefsReceived(BMessage* msg);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun(void);

private:
			status_t			_Warn(const char* url);
			status_t			_DecodeUrlString(BString& string);
};

#endif // _URL_WRAPPER_H

