/*
 * Copyright 2003-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef PNG_VIEW_H
#define PNG_VIEW_H


#include "TranslatorSettings.h"

#include <View.h>

class BPopUpMenu;


// Config panel messages
#define M_PNG_SET_INTERLACE	'pnsi'

// default view size
#define PNG_VIEW_WIDTH		300
#define PNG_VIEW_HEIGHT		270


class PNGView : public BView {
	public:
		PNGView(const BRect &frame, const char *name, uint32 resizeMode,
			uint32 flags, TranslatorSettings *settings);
		~PNGView();

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		BMessage* _InterlaceMessage(int32 kind);

	private:
		BPopUpMenu*			fInterlaceMenu;
		TranslatorSettings*	fSettings;
			// the actual settings for the translator,
			// shared with the translator
};

#endif	// PNG_VIEW_H
