/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_USAGE_CONDITIONS_WINDOW_H
#define USER_USAGE_CONDITIONS_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "HaikuDepotConstants.h"
#include "PackageInfo.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"


class BarberPole;
class BTextView;
class BStringView;
class MarkupTextView;
class Model;


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
									UserUsageConditionsSelectionMode mode,
									const UserDetail& userDetail);
	static float				_ExpectedIntroductionTextHeight(
									BTextView* introductionTextView);

	void						_DisplayData(const UserDetail& userDetail,
									const UserUsageConditions&
									userUsageConditions);

	void						_FetchData();
	void						_SetWorkerThread(thread_id thread);
	static int32				_FetchDataThreadEntry(void* data);
	void						_FetchDataPerform();
	status_t					_FetchUserUsageConditionsCodePerform(
									UserDetail& userDetail, BString& code);
	status_t					_FetchUserUsageConditionsCodeForUserPerform(
									UserDetail& userDetail, BString& code);
	void						_NotifyFetchProblem();

private:
			UserUsageConditionsSelectionMode
								fMode;
			MarkupTextView*		fCopyView;
			Model&				fModel;
			BStringView*		fAgeNoteStringView;
			BStringView*		fVersionStringView;
			BTextView*			fIntroductionTextView;
			BarberPole*			fWorkerIndicator;
			thread_id			fWorkerThread;
};


#endif // USER_USAGE_CONDITIONS_WINDOW_H
