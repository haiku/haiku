/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef DECOR_MANAGER_H
#define DECOR_MANAGER_H


#include <image.h>
#include <String.h>
#include <Locker.h>
#include <ObjectList.h>

#include "Decorator.h"

class DecorInfo;
class Desktop;
class DrawingEngine;

class DecorManager : public BLocker {
	public:
					DecorManager();
					~DecorManager();

		void		RescanDecorators();

		Decorator*	AllocateDecorator(Desktop* desktop,
						DrawingEngine* engine,
						BRect rect,
						const char* title, window_look look,
						uint32 flags);

		int32		CountDecorators() const;

		int32		GetDecorator() const;
		bool		SetDecorator(int32 index);
		bool		SetR5Decorator(int32 value);
		const char*	GetDecoratorName(int32 index);

		// TODO: Implement this method once the rest of the necessary infrastructure
		// is in place
		//status_t	GetPreview(int32 index, ServerBitmap *bitmap);

	private:
		void		_EmptyList();
		DecorInfo*	_FindDecor(const char *name);

		BObjectList<DecorInfo> fDecorList;
		DecorInfo*	fCurrentDecor;
};

extern DecorManager gDecorManager;

#endif	/* DECOR_MANAGER_H */
