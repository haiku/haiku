/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Doerfler, axeld@pinc-software.de
 */
#ifndef TMWINDOW_H
#define TMWINDOW_H


#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <Window.h>

#include "TMListItem.h"


class TMDescView : public BBox {
	public:
		TMDescView();
		virtual ~TMDescView();

		virtual void MessageReceived(BMessage* message);
		virtual void Draw(BRect bounds);
		virtual void GetPreferredSize(float *_width, float *_height);

		void CtrlAltDelPressed(bool keyDown);

		void SetItem(TMListItem *item);
		TMListItem *Item() { return fItem; }

	private:
		const char* fText[3];
		TMListItem* fItem;
		int32 fSeconds;
		BMessageRunner* fRebootRunner;
};

class TMWindow : public BWindow {
	public:
		TMWindow();
		virtual ~TMWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

		void Enable();
		void Disable();

	private:
		void UpdateList();

		bool fQuitting;

		BMessageRunner* fUpdateRunner;
		BView *fView;
		BListView *fListView;
		BButton *fKillButton;
		BButton *fRestartButton;
		TMDescView *fDescriptionView;
};

static const uint32 kMsgCtrlAltDelPressed = 'TMcp';

#endif	// TMWINDOW_H
