/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TO_LATEST_USER_USAGE_CONDITIONS_WINDOW_H
#define TO_LATEST_USER_USAGE_CONDITIONS_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "BarberPole.h"
#include "HaikuDepotConstants.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"

class BButton;
class BCheckBox;
class BTextView;
class LinkView;
class Model;


class ToLatestUserUsageConditionsWindow : public BWindow {
public:
								ToLatestUserUsageConditionsWindow(
									BWindow* parent,
									Model& model, const UserDetail& userDetail);
	virtual						~ToLatestUserUsageConditionsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_EnableMutableControls();
			void				_InitUiControls();

			void				_DisplayData(const UserUsageConditions&
									userUsageConditions);
			void				_HandleViewUserUsageConditions();
			void				_HandleLogout();
			void				_HandleAgree();
			void				_HandleAgreeFailed();

			void				_SetWorkerThread(thread_id thread);
			void				_SetWorkerThreadLocked(thread_id thread);

			void				_FetchData();
	static	int32				_FetchDataThreadEntry(void* data);
			void				_FetchDataPerform();
			void				_NotifyFetchProblem();

			void				_Agree();
	static	int32				_AgreeThreadEntry(void* data);
			void				_AgreePerform();

private:
			UserUsageConditions	fUserUsageConditions;
			Model&				fModel;
			UserDetail			fUserDetail;

			BTextView*			fMessageTextView;
			BButton*			fLogoutButton;
			BButton*			fAgreeButton;
			BCheckBox*			fConfirmMinimumAgeCheckBox;
			BCheckBox*			fConfirmUserUsageConditionsCheckBox;
			LinkView*			fUserUsageConditionsLink;
			BarberPole*			fWorkerIndicator;

			BLocker				fLock;
			thread_id			fWorkerThread;
			bool				fQuitRequestedDuringWorkerThread;
			bool				fMutableControlsEnabled;
};


#endif // TO_LATEST_USER_USAGE_CONDITIONS_WINDOW_H
