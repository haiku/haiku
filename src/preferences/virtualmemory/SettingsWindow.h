/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2010-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hamish Morrison, hamish@lavabit.com
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H


#include <MenuItem.h>
#include <Slider.h>
#include <Volume.h>
#include <Window.h>

#include "Settings.h"


class BStringView;
class BCheckBox;
class BSlider;
class BButton;
class BMenuField;


class SizeSlider : public BSlider {
public:
							SizeSlider(const char* name, const char* label,
								BMessage* message, int32 min, int32 max,
								uint32 flags);
	virtual 				~SizeSlider() {};

	virtual	const char*		UpdateText() const;

private:
	mutable	char			fText[128];
};


class VolumeMenuItem : public BMenuItem, public BHandler {
public:
							VolumeMenuItem(BVolume volume, BMessage* message);
	virtual					~VolumeMenuItem() {}

	virtual	BVolume			Volume() { return fVolume; }
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			GenerateLabel();

private:
			BVolume			fVolume;
};


class SettingsWindow : public BWindow {
public:
							SettingsWindow();
	virtual					~SettingsWindow() {};

	virtual void			MessageReceived(BMessage* message);
	virtual bool			QuitRequested();

private:
			status_t		_AddVolumeMenuItem(dev_t device);
			status_t		_RemoveVolumeMenuItem(dev_t device);
			VolumeMenuItem*	_FindVolumeMenuItem(dev_t device);

			void			_Update();

			BCheckBox*		fSwapEnabledCheckBox;
			BCheckBox*		fSwapAutomaticCheckBox;
			BSlider*		fSizeSlider;
			BButton*		fDefaultsButton;
			BButton*		fRevertButton;
			BStringView*	fWarningStringView;
			BMenuField*		fVolumeMenuField;
			Settings		fSettings;
};

#endif	/* SETTINGS_WINDOW_H */
