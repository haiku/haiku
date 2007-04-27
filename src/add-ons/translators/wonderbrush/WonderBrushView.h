/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef WONDERBRUSH_VIEW_H
#define WONDERBRUSH_VIEW_H

#include <View.h>
#include "TranslatorSettings.h"

class BMenuField;

class WonderBrushView : public BView {
public:
	WonderBrushView(const BRect &frame, const char *name, uint32 resize,
		uint32 flags, TranslatorSettings *settings);
		// sets up the view
		
	~WonderBrushView();
		// releases the WonderBrushTranslator settings

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);

	virtual	void Draw(BRect area);
		// draws information about the WonderBrushTranslator 
	virtual	void GetPreferredSize(float* width, float* height);

	enum {
		MSG_COMPRESSION_CHANGED	= 'cmch',
	};

private:
	TranslatorSettings *fSettings;
		// the actual settings for the translator,
		// shared with the translator
};

#endif // WONDERBRUSH_VIEW_H
