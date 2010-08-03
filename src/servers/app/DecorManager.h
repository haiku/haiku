/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
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
class DesktopListener;
class DrawingEngine;
class Window;
class WindowBehaviour;


typedef BObjectList<DesktopListener> DesktopListenerList;


class DecorAddOn {
public:
								DecorAddOn(image_id id, const char* name);
	virtual						~DecorAddOn();

	virtual status_t			InitCheck() const;

		image_id				ImageID() const { return fImageID; }
		BString					Name() const { return fName; }

		Decorator*				AllocateDecorator(Desktop* desktop,
									DrawingEngine* engine, BRect rect,
									const char* title, window_look look,
									uint32 flags);

	virtual float				Version() { return 1.0; }
	virtual WindowBehaviour*	AllocateWindowBehaviour(Window* window);

	virtual const DesktopListenerList&	GetDesktopListeners();

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);

		DesktopListenerList		fDesktopListeners;

private:
		image_id				fImageID;
		BString 				fName;
};


class DecorManager
{
public:
					DecorManager();
					~DecorManager();

		void		RescanDecorators();

		Decorator*	AllocateDecorator(Desktop* desktop,
						DrawingEngine* engine,
						BRect rect,
						const char* title, window_look look,
						uint32 flags);
		WindowBehaviour*			AllocateWindowBehaviour(Window* window);
		const DesktopListenerList&	GetDesktopListeners();

		int32		CountDecorators() const;

		int32		GetDecorator() const;
		bool		SetDecorator(int32 index, Desktop* desktop);
		bool		SetR5Decorator(int32 value);
		BString		GetDecoratorName(int32 index);

		// TODO: Implement this method once the rest of the necessary infrastructure
		// is in place
		//status_t	GetPreview(int32 index, ServerBitmap *bitmap);

private:
		void		_EmptyList();
		DecorAddOn*	_FindDecor(BString name);

		bool		_LoadSettingsFromDisk();
		bool		_SaveSettingsToDisk();

		BObjectList<DecorAddOn> fDecorList;
		DecorAddOn	fDefaultDecorAddOn;
		DecorAddOn*	fCurrentDecor;
};

extern DecorManager gDecorManager;

#endif	/* DECOR_MANAGER_H */
