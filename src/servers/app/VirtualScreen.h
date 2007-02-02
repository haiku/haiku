/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef VIRTUAL_SCREEN_H
#define VIRTUAL_SCREEN_H


#include "ScreenManager.h"

#include <Message.h>

class Desktop;
class DrawingEngine;
class HWInterface;


class VirtualScreen {
	public:
		VirtualScreen();
		~VirtualScreen();

		::DrawingEngine*	DrawingEngine() const
								{ return fDrawingEngine; }
		// TODO: can we have a multiplexing HWInterface as well?
		//	If not, this would need to be hidden, and only made
		//	available for the Screen class
		::HWInterface*		HWInterface() const
								{ return fHWInterface; }

		status_t			RestoreConfiguration(Desktop& desktop, const BMessage* settings);
		status_t			StoreConfiguration(BMessage& settings);

		status_t			AddScreen(Screen* screen);
		status_t			RemoveScreen(Screen* screen);

		void				UpdateFrame();
		BRect				Frame() const;

		// TODO: we need to play with a real multi-screen configuration to
		//	figure out the specifics here - possibly in the test environment?
		void				SetScreenFrame(int32 index, BRect frame);

		Screen*				ScreenAt(int32 index) const;
		BRect				ScreenFrameAt(int32 index) const;
		int32				CountScreens() const;

	private:
		status_t			_FindConfiguration(Screen* screen, BMessage& settings);
		void				_Reset();

		struct screen_item {
			Screen*	screen;
			BRect	frame;
			// TODO: do we want to have a different color per screen as well?
		};

		BMessage			fSettings;
		BRect				fFrame;
		BObjectList<screen_item> fScreenList;
		::DrawingEngine*	fDrawingEngine;
		::HWInterface*		fHWInterface;
};

#endif	/* VIRTUAL_SCREEN_H */
