/*
 * Copyright 2010-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <View.h>

class BCheckBox;
class BPopUpMenu;
class BSlider;
class TranslatorSettings;

class ConfigView : public BView {
public:
							ConfigView(TranslatorSettings* settings,
								uint32 flags = B_WILL_DRAW);
	virtual 				~ConfigView();

	virtual void 			AttachedToWindow();
	virtual void 			MessageReceived(BMessage *message);

private:
		TranslatorSettings*	fSettings;
		BPopUpMenu*			fPresetsMenu;
		BSlider*			fQualitySlider;
		BSlider*			fMethodSlider;
		BCheckBox*			fPreprocessingCheckBox;
};


#endif	/* CONFIG_VIEW_H */
