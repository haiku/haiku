/*
 * Copyright 2002-2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include "Prefs.h"

#include <Box.h>
#include <ColorControl.h>

class BCheckBox;
class BRadioButton;
class BTextControl;


class RTColorControl : public BColorControl {
	public:
		RTColorControl(BPoint point, BMessage *message);
		void SetValue(int32 color);
};

class ConfigView : public BBox {
	public:
		ConfigView(BRect rect, const char *name, uint32 mode,
			BMessenger& target, Prefs *prefs);

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void GetPreferredSize(float* _width, float* _height);

		void UpdateDeskbarIconWidth();

	private:
		void _ResetDefaults();

		int32			fMode;
		BMessenger		fTarget;
		Prefs*			fPrefs;
		RTColorControl* fColorControl;

		bool			fFirstTimeAttached;

		// For Normal
		BCheckBox*		fFadeCheckBox;
		// For Mini and Deskbar
		BRadioButton*	fActiveButton;
		BRadioButton*	fIdleButton;
		BRadioButton*	fFrameButton;
		// For Deskbar
		BTextControl*	fIconWidthControl;
};

#endif	// CONFIG_VIEW_H
