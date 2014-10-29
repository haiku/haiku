/*
 * Copyright 2003-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 *		Atsushi Takamatsu
 */
#ifndef __HWINDOW_H__
#define __HWINDOW_H__


#include <Window.h>
#include <FilePanel.h>
#include <FileGameSound.h>


class HEventList;
class HTypeList;


enum{
	M_PLAY_MESSAGE = 'MPLM',
	M_STOP_MESSAGE = 'MSTO',
	M_REMOVE_MESSAGE = 'MREM',
	M_ITEM_MESSAGE = 'MITE',
	M_OTHER_MESSAGE = 'MOTH',
	M_NONE_MESSAGE = 'MNON',
	M_ADD_EVENT = 'MADE',
	M_REMOVE_EVENT = 'MREE',
	M_OPEN_WITH = 'MOPW'
};


class HWindow : public BWindow {
public:
								HWindow(BRect rect, const char* name);
	virtual						~HWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_InitGUI();
			void				_Pulse();
			void				_SetupMenuField();
			void				_UpdateZoomLimits();

private:
			HEventList*			fEventList;
			BFilePanel*			fFilePanel;
			BFileGameSound*		fPlayer;
			BRect				fFrame;
};


#endif	// __HWINDOW_H__
