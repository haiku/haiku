/*
 * Copyright 2005-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef VIRTUAL_SCREEN_H
#define VIRTUAL_SCREEN_H


#include "ScreenConfigurations.h"
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

			status_t			SetConfiguration(Desktop& desktop,
									ScreenConfigurations& configurations,
									uint32* _changedScreens = NULL);

			status_t			AddScreen(Screen* screen,
									ScreenConfigurations& configurations);
			status_t			RemoveScreen(Screen* screen);

			void				UpdateFrame();
			BRect				Frame() const;

			// TODO: we need to play with a real multi-screen configuration to
			//	figure out the specifics here
			void				SetScreenFrame(int32 index, BRect frame);

			Screen*				ScreenAt(int32 index) const;
			Screen*				ScreenByID(int32 id) const;
			BRect				ScreenFrameAt(int32 index) const;
			int32				CountScreens() const;

private:
			status_t			_GetMode(Screen* screen,
									ScreenConfigurations& configurations,
									display_mode& mode) const;
			void				_Reset();

	struct screen_item {
		Screen*	screen;
		BRect	frame;
		// TODO: do we want to have a different color per screen as well?
	};

			BRect				fFrame;
			BObjectList<screen_item> fScreenList;
			::DrawingEngine*	fDrawingEngine;
			::HWInterface*		fHWInterface;
};

#endif	/* VIRTUAL_SCREEN_H */
