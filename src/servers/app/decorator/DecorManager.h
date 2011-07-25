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
#include <Entry.h>
#include <DecorInfo.h>

#include "Decorator.h"

class Desktop;
class DesktopListener;
class DrawingEngine;
class Window;
class WindowBehaviour;


typedef BObjectList<DesktopListener> DesktopListenerList;


// special name to test for use of non-fs-tied default decorator
// this just keeps things clean and simple is all

class DecorAddOn {
public:
								DecorAddOn(image_id id, const char* name);
	virtual						~DecorAddOn();

	virtual status_t			InitCheck() const;

			image_id			ImageID() const { return fImageID; }

			Decorator*			AllocateDecorator(Desktop* desktop,
									DrawingEngine* engine, BRect rect,
									const char* title, window_look look,
									uint32 flags);

	virtual	WindowBehaviour*	AllocateWindowBehaviour(Window* window);

	virtual const DesktopListenerList& GetDesktopListeners();

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);

			DesktopListenerList	fDesktopListeners;

private:
			image_id			fImageID;
			BString 			fName;
};


class DecorManager {
public:
								DecorManager();
								~DecorManager();

			Decorator*			AllocateDecorator(Window *window);
			WindowBehaviour*	AllocateWindowBehaviour(Window *window);
			void				CleanupForWindow(Window *window);

			status_t			PreviewDecorator(BString path, Window *window);

			const DesktopListenerList& GetDesktopListeners();

			BString 			GetCurrentDecorator() const;
			status_t			SetDecorator(BString path, Desktop *desktop);

private:
			DecorAddOn*			_LoadDecor(BString path, status_t &error);
			bool				_LoadSettingsFromDisk();
			bool				_SaveSettingsToDisk();

private:
			DecorAddOn			fDefaultDecor;
			DecorAddOn*			fCurrentDecor;
			DecorAddOn*			fPreviewDecor;

			Window*				fPreviewWindow;
			BString				fCurrentDecorPath;
};

extern DecorManager gDecorManager;

#endif	/* DECOR_MANAGER_H */
