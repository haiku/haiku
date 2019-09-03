/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_USAGE_CONDITIONS_WINDOW_H
#define USER_USAGE_CONDITIONS_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "PackageInfo.h"
#include "UserUsageConditions.h"


class BarberPole;
class BTextView;
class BStringView;
class MarkupTextView;
class Model;


enum UserUsageConditionsSelectionMode {
	LATEST		= 1,
	USER		= 2,
	FIXED		= 3
		// means that the user usage conditions are supplied to the window.
};


class UserUsageConditionsWindow : public BWindow {
public:
								UserUsageConditionsWindow(Model& model,
									UserUsageConditions& userUsageConditions);
								UserUsageConditionsWindow(Model& model,
									UserUsageConditionsSelectionMode mode);
	virtual						~UserUsageConditionsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

private:
	void						_InitUiControls();

	static const BString		_VersionText(const BString& code);
	static const BString		_MinimumAgeText(uint8 minimumAge);
	static const BString		_IntroductionTextForMode(
									UserUsageConditionsSelectionMode mode);
	static float				_ExpectedIntroductionTextHeight(
									BTextView* introductionTextView);

	void						_DisplayData(const UserUsageConditions& data);

	void						_FetchData();
	void						_SetWorkerThread(thread_id thread);
	static int32				_FetchDataThreadEntry(void* data);
	void						_FetchDataPerform();

private:
			UserUsageConditionsSelectionMode
								fMode;
			MarkupTextView*		fCopyView;
			Model&				fModel;
			BStringView*		fAgeNoteStringView;
			BStringView*		fVersionStringView;
			BarberPole*			fWorkerIndicator;
			thread_id			fWorkerThread;
};


#endif // USER_USAGE_CONDITIONS_WINDOW_H
