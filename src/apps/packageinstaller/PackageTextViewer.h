/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGETEXTVIEWER_H
#define PACKAGETEXTVIEWER_H

#include <View.h>
#include <TextView.h>

#include "BlockingWindow.h"

class PackageTextViewer : public BlockingWindow {
public:
								PackageTextViewer(const char* text,
									bool disclaimer = false);
		
	virtual	void				MessageReceived(BMessage* message);
		
private:
			void				_InitView(const char *text, bool disclaimer);
};


#endif

