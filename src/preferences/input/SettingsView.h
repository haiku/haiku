/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <PopUpMenu.h>
#include <Slider.h>


class MouseSettings;
class MouseView;


class SettingsView : public BBox {
	public:
								SettingsView(MouseSettings &settings);
		virtual 				~SettingsView();

		virtual void 			AttachedToWindow();

				void 			SetMouseType(int32 type);
				void 			MouseMapUpdated();
				void 			UpdateFromSettings();

	public:
				// FIXME use proper getters/setters for this?
				BCheckBox*		fAcceptFirstClickBox;

	private:
		typedef	BBox			inherited;

		const	MouseSettings&	fSettings;

				BOptionPopUp*	fTypeMenu;
				BOptionPopUp*	fFocusMenu;
				MouseView*		fMouseView;
				BSlider*		fClickSpeedSlider;
				BSlider*		fMouseSpeedSlider;
				BSlider*		fAccelerationSlider;
};

#endif	/* SETTINGS_VIEW_H */
