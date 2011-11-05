/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <Dragger.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <String.h>
#include <View.h>

#include "CPUFrequencyView.h"
#include "DriverInterface.h"
#include "PreferencesWindow.h"


extern const char* kDeskbarItemName;
extern const char* kAddonSignature;


class FrequencySwitcher : public BMessageFilter {
public:
								FrequencySwitcher(
									CPUFreqDriverInterface* interface,
									BHandler* target);
	virtual						~FrequencySwitcher();
		
	virtual filter_result		Filter(BMessage* message, BHandler** _target);

			void				SetMode(const freq_preferences& pref);
		
private:
			void				_CalculateDynamicState();
			void				_StartDynamicPolicy(bool start,
									const freq_preferences& pref);
		
			CPUFreqDriverInterface*	fDriverInterface;
			BHandler*			fTarget;
			BMessageRunner*		fMessageRunner;
			
			freq_info*			fCurrentFrequency;
			
			bool				fDynamicPolicyStarted;
			bigtime_t 			fPrevActiveTime;
			bigtime_t 			fPrevTime;
			float				fSteppingThreshold;
			bigtime_t			fIntegrationTime;
};


class FrequencyMenu : public BMessageFilter {
public:
								FrequencyMenu(BMenu* menu, BHandler* target,
									PreferencesStorage<freq_preferences>*
										storage,
									CPUFreqDriverInterface* interface);
	virtual filter_result		Filter(BMessage* message, BHandler** _target);

			void				UpdateMenu();

private:
	inline	void				_SetL1MenuLabelFrom(BMenuItem* item);
		
			BHandler*			fTarget;
			BMenuItem*			fDynamicPerformance;
			BMenuItem*			fHighPerformance;
			BMenuItem*			fLowEnergie;
			BMenu*				fCustomStateMenu;
			
			PreferencesStorage<freq_preferences>* fStorage;
			CPUFreqDriverInterface* fInterface;
};


class StatusView : public BView {
public:
								StatusView(BRect frame,	bool inDeskbar = false,
									PreferencesStorage<freq_preferences>*
										storage = NULL);
								StatusView(BMessage* archive);
	virtual						~StatusView();

	static	StatusView*			Instantiate(BMessage* archive);
	virtual	status_t 			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				FrameResized(float width, float height);
	virtual	void				MouseDown(BPoint where);
	virtual	void				Draw(BRect updateRect);

	virtual void				GetPreferredSize(float *width, float *height);
	virtual void				ResizeToPreferred(void);
	
	virtual void				ShowPopUpMenu(bool show = true);
	
	virtual void				UpdateCPUFreqState();
		
private:
			void				_Init();
			void				_SetupNewFreqString();
			void				_OpenPreferences();
			void				_AboutRequested();
			void 				_Quit();
			
			bool				fInDeskbar;
			
			CPUFreqDriverInterface	fDriverInterface;
			freq_info*			fCurrentFrequency;
			FrequencySwitcher*	fFrequencySwitcher;
			
			bool				fShowPopUpMenu;
			BPopUpMenu*			fPreferencesMenu;
			BMenuItem*			fOpenPrefItem;
			BMenuItem*			fQuitItem;
			
			FrequencyMenu*		fPreferencesMenuFilter;
			bool				fOwningStorage;
			PreferencesStorage<freq_preferences>* fStorage;
			PrefFileWatcher<freq_preferences>* fPrefFileWatcher;
			
			BString				fFreqString;
			BDragger*			fDragger;
};

#endif	// STATUS_VIEW_H
