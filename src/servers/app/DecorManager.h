/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef DECORMANAGER_H
#define DECORMANAGER_H

#include <image.h>
#include <String.h>
#include <Locker.h>
#include <List.h>

#include "Decorator.h"
#include "DisplayDriver.h"

class DecorInfo;

class DecorManager : public BLocker
{
public:
					DecorManager(void);
					~DecorManager(void);
	
	void			RescanDecorators(void);
	
	Decorator *		AllocateDecorator(BRect rect, const char *title, 
									int32 wlook, int32 wfeel,
									int32 wflags, DisplayDriver *ddriver);
	
	int32			CountDecorators(void) const;
	
	int32			GetDecorator(void) const;
	bool			SetDecorator(const int32 &index);
	bool			SetR5Decorator(const int32 &value);
	const char *	GetDecoratorName(const int32 &index);
	
	// TODO: Implement this method once the rest of the necessary infrastructure
	// is in place
//	status_t		GetPreview(const int32 &index, ServerBitmap *bitmap);
	
private:
	void		EmptyList(void);
	DecorInfo*	FindDecor(const char *name);
	
	BList		fDecorList;
	DecorInfo*	fCurrentDecor;
};

extern DecorManager gDecorManager;

#endif
