/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
 
#ifndef CPUFREQUENCYVIEW_h
#define CPUFREQUENCYVIEW_h

#include "DriverInterface.h"
#include "ColorStepView.h"
#include "PreferencesWindow.h"

#include <Handler.h>
#include <Menu.h>
#include <MenuItem.h>
#include <TextControl.h>


extern const char* kPrefSignature;
extern const char* kPreferencesFileName;

enum stepping_mode {
	DYNAMIC,
	PERFORMANCE,
	LOW_ENERGIE,
	CUSTOM
};


struct freq_preferences
{
	bool IsEqual(const freq_preferences& prefs) {
		if (mode == prefs.mode && custom_stepping == prefs.custom_stepping
			&& stepping_threshold == prefs.stepping_threshold
			&& integration_time == prefs.integration_time)
			return true;
		return false;
	}

	// stepping mode
	stepping_mode	mode;
	int16			custom_stepping;
		
	// dynamic stepping
	float			stepping_threshold;
	bigtime_t		integration_time;
};


const freq_preferences kDefaultPreferences =
{
	DYNAMIC,
	-1,
	0.25,
	500000	//half second
};


class FrequencyMenu;
class StatusView;

class CPUFrequencyView : public BView
{
	public:
								CPUFrequencyView(BRect frame,
													PreferencesStorage<freq_preferences>* storage);
		virtual	void			MessageReceived(BMessage* message);
		
		virtual	void			AttachedToWindow();
		virtual	void			DetachedFromWindow();
		
	private:
		void					_InstallReplicantInDeskbar();
		void					_UpdateViews();
		void					_ReadIntegrationTime();
		
		BMenu*					fPolicyMenu;
		FrequencyMenu*			fFrequencyMenu;
		ColorStepView*			fColorStepView;
		BTextControl*			fIntegrationTime;
		StatusView*				fStatusView;
		BButton*				fInstallButton;
		
		PreferencesStorage<freq_preferences>*	fStorage;
		CPUFreqDriverInterface	fDriverInterface;
};

#endif
