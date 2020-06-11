/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GENERAL_VIEW_H
#define _GENERAL_VIEW_H


#include <Button.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuField.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>

#include "SettingsPane.h"


class GeneralView : public SettingsPane {
public:
							GeneralView(SettingsHost* host);

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* msg);

			// SettingsPane hooks
			status_t		Load(BMessage&);
			status_t		Save(BMessage&);
			status_t		Revert();
			bool			RevertPossible();
			status_t		Defaults();
			bool			DefaultsPossible();
			bool			UseDefaultRevertButtons();

private:
		BCheckBox*			fNotificationBox;
		BSlider*			fDurationSlider;
		BSlider*			fWidthSlider;
		BPopUpMenu*			fPositionMenu;

		
		int32				fOriginalTimeout;
		float				fOriginalWidth;
		icon_size			fOriginalIconSize;
		uint32				fOriginalPosition;
		uint32				fNewPosition;

		void				_EnableControls();
		void				_SetTimeoutLabel(int32 value);
		bool				_IsServerRunning();
};

#endif // _GENERAL_VIEW_H
