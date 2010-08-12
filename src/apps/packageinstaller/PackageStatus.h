/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGESTATUS_H
#define PACKAGESTATUS_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StatusBar.h>


enum {
	P_MSG_NEXT_STAGE = 'psne',
	P_MSG_STOP,
	P_MSG_RESET
};


class StopButton : public BButton {
	public:
		StopButton();
		virtual void Draw(BRect);
};


class PackageStatus : public BWindow {
	public:
		PackageStatus(const char *title, const char *label = NULL,
				const char *trailing = NULL, BHandler *parent = NULL);
		~PackageStatus();
		
		void MessageReceived(BMessage *msg);
		void Reset(uint32 stages, const char *label = NULL,
				const char *trailing = NULL);
		void StageStep(uint32 count, const char *text = NULL, 
				const char *trailing = NULL);
		bool Stopped();
		
	private:
		BStatusBar *fStatus;
		StopButton *fButton;
		bool fIsStopped;
		BHandler *fParent;
};


#endif

