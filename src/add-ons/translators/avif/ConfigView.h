/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Emmanuel Gil Peyrot
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <GroupView.h>

class BCheckBox;
class BPopUpMenu;
class BSlider;
class TranslatorSettings;

class ConfigView : public BGroupView {
public:
					ConfigView(TranslatorSettings* settings);
	virtual 		~ConfigView();

	virtual void 	AttachedToWindow();
	virtual void 	MessageReceived(BMessage *message);

private:
	TranslatorSettings*	fSettings;
	BPopUpMenu*			fPixelFormatMenu;
	BCheckBox*			fLosslessCheckBox;
	BSlider*			fQualitySlider;
	BSlider*			fSpeedSlider;
	BSlider*			fHTilesSlider;
	BSlider*			fVTilesSlider;
};


#endif	/* CONFIG_VIEW_H */
