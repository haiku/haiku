/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGETEXTVIEWER_H
#define PACKAGETEXTVIEWER_H

#include <Window.h>
#include <View.h>
#include <TextView.h>


class PackageTextViewer : public BWindow {
	public:
		PackageTextViewer(const char *text, bool disclaimer = false);
		~PackageTextViewer();
		
		int32 Go();

		void MessageReceived(BMessage *msg);
		
	private:
		void _InitView(const char *text, bool disclaimer);

		BView *fBackground;
		BTextView *fText;

		sem_id fSemaphore;
		int32 fValue;
};


#endif

