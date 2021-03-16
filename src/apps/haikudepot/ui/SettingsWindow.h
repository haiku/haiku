/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "BarberPole.h"
#include "HaikuDepotConstants.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"


class BButton;
class BCheckBox;
class Model;


class SettingsWindow : public BWindow {
public:
								SettingsWindow(BWindow* parent, Model* model);
	virtual						~SettingsWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_InitUiControls();
			void				_UpdateUiFromModel();
			void				_UpdateModelFromUi();

private:
			Model*				fModel;

			BCheckBox*			fCanShareAnonymousUsageDataCheckBox;

			BButton*			fApplyButton;
			BButton*			fCancelButton;
};


#endif // SETTINGS_WINDOW_H
