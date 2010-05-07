/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef HVIF_VIEW_H
#define HVIF_VIEW_H

#include "TranslatorSettings.h"

#include <Slider.h>
#include <View.h>

class HVIFView : public BView {
public:
	HVIFView(const char *name, uint32 flags, TranslatorSettings *settings);
	~HVIFView();

	virtual	void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage *message);

private:
	BSlider*			fRenderSize;
	TranslatorSettings* fSettings;
};

#endif	// HVIF_VIEW_H
