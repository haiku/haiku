/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Brecht Machiels (brecht@mos6581.org)
 */
#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>
#include <PopUpMenu.h>


class MouseSettings;
class MouseView;


class SettingsView : public BBox {
	public:
		SettingsView(MouseSettings &settings);
		virtual ~SettingsView();

		virtual void AttachedToWindow();

		void SetMouseType(int32 type);
		void MouseMapUpdated();
		void UpdateFromSettings();

	private:
		friend class MouseWindow;

		typedef BBox inherited;

		const MouseSettings &fSettings;

		BPopUpMenu*	fTypeMenu;
		BPopUpMenu*	fFocusMenu;
		BPopUpMenu*	fFocusFollowsMouseMenu;
		BCheckBox*	fAcceptFirstClickBox;
		MouseView*	fMouseView;
		BSlider*	fClickSpeedSlider;
		BSlider*	fMouseSpeedSlider;
		BSlider*	fAccelerationSlider;
};

#endif	/* SETTINGS_VIEW_H */
