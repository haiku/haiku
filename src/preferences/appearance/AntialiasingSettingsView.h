/*
 * Copyright 2008, Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ANTIALIASING_SETTINGS_VIEW_H
#define ANTIALIASING_SETTINGS_VIEW_H


#include <View.h>

class BBox;
class BMenuField;
class BPopUpMenu;
class BSlider;
class BTextView;


class AntialiasingSettingsView : public BView {
public:
							AntialiasingSettingsView(const char* name);
	virtual					~AntialiasingSettingsView();

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);

			void			SetDefaults();
			void			Revert();
			bool			IsDefaultable();
			bool			IsRevertable();

private:
			void			_BuildAntialiasingMenu();
			void			_SetCurrentAntialiasing();
			void			_BuildHintingMenu();
			void			_SetCurrentHinting();
			void			_SetCurrentAverageWeight();

protected:
			float			fDivider;

			BMenuField*		fAntialiasingMenuField;
			BPopUpMenu*		fAntialiasingMenu;
			BMenuField*		fHintingMenuField;
			BPopUpMenu*		fHintingMenu;
			BSlider*		fAverageWeightControl;

			bool			fSavedSubpixelAntialiasing;
			bool			fCurrentSubpixelAntialiasing;
			uint8			fSavedHinting;
			uint8			fCurrentHinting;
			unsigned char	fSavedAverageWeight;
			unsigned char	fCurrentAverageWeight;
};

#endif // ANTIALIASING_SETTINGS_VIEW_H
