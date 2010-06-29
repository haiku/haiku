/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __MAIN_WIN_H
#define __MAIN_WIN_H

#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <Menu.h>
#include <Slider.h>
#include <Window.h>

#include "Controller.h"
#include "VideoView.h"

class MainWin : public BWindow
{
public:
						MainWin(BRect rect);
						~MainWin();

	void				FrameResized(float new_width, float new_height);
	void				Zoom(BPoint rec_position, float rec_width, float rec_height);
	void				DispatchMessage(BMessage *message, BHandler *handler);
	void				MessageReceived(BMessage *msg);
	bool				QuitRequested();

	void				MouseDown(BMessage *msg);
	void				MouseMoved(BMessage *msg);
	void				MouseUp(BMessage *msg);
	status_t			KeyDown(BMessage *msg);

	void				CreateMenu();
	
	void				SetupInterfaceMenu();
	void				SetupChannelMenu();
	
	void				SelectInitialInterface();
	
	void				SelectInterface(int i);
	void				SelectChannel(int i);

	void				SetInterfaceMenuMarker();
	void				SetChannelMenuMarker();

	void				VideoFormatChange(int width, int height, float width_scale, float height_scale);

	void				UpdateWindowTitle();
	
	void				AdjustFullscreenRenderer();
	void 				AdjustWindowedRenderer(bool user_resized);
	
	void				ToggleFullscreen();
	void				ToggleKeepAspectRatio();
	void				ToggleAlwaysOnTop();
	void				ToggleNoBorder();
	void				ToggleNoMenu();
	void				ToggleNoBorderNoMenu();

	void				ShowContextMenu(const BPoint &screen_point);
	
	BMenuBar *			fMenuBar;
	BView *				fBackground;
	VideoView *			fVideoView;

	BMenu *				fFileMenu;
	BMenu *				fChannelMenu;
	BMenu *				fInterfaceMenu;
	BMenu *				fSettingsMenu;
	BMenu *				fDebugMenu;

	Controller *		fController;
	volatile bool		fIsFullscreen;
	volatile bool		fKeepAspectRatio;
	volatile bool		fAlwaysOnTop;
	volatile bool		fNoMenu;
	volatile bool		fNoBorder;
	int					fSourceWidth;
	int					fSourceHeight;
	float				fWidthScale;
	float				fHeightScale;
	int					fMenuBarHeight;
	BRect				fSavedFrame;
	bool				fMouseDownTracking;
	BPoint				fMouseDownMousePos;
	BPoint				fMouseDownWindowPos;
	bool				fFrameResizedTriggeredAutomatically;
	bool				fIgnoreFrameResized;
	bool				fFrameResizedCalled;
};

#endif
