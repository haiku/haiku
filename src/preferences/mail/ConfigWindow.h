/*
 * Copyright 2004-2015, Haiku Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef CONFIG_WINDOW_H
#define CONFIG_WINDOW_H


#include <Window.h>
#include <ObjectList.h>
#include <ListItem.h>

#include "MailSettings.h"


class BPopup;
class BTextControl;
class BCheckBox;
class BListView;
class BButton;
class BMenuField;
class BMailSettings;
class BTextView;
class CenterContainer;


enum item_types {
	ACCOUNT_ITEM = 0,
	INBOUND_ITEM,
	OUTBOUND_ITEM,
	FILTER_ITEM
};


class AccountItem : public BStringItem {
public:
								AccountItem(const char* label,
									BMailAccountSettings* account,
									item_types type);

			void				Update(BView* owner, const BFont* font);
			void				DrawItem(BView* owner, BRect rect,
									bool complete);
			BMailAccountSettings* Account() { return fAccount; }
			item_types			Type() { return fType; }

private:
			BMailAccountSettings* fAccount;
			item_types			fType;
};


class ConfigWindow : public BWindow {
public:
								ConfigWindow();
								~ConfigWindow();

			bool				QuitRequested();
			void				MessageReceived(BMessage* msg);

			BMailAccountSettings*	AddAccount();
			void				AccountUpdated(BMailAccountSettings* account);

private:
			BView*				_BuildHowToView();

			void				_LoadSettings();
			void				_LoadAccounts();
			void				_SaveSettings();

			status_t			_SetToGeneralSettings(BMailSettings *general);
			void				_RevertToLastSettings();

			void				_AddAccountToView(
									BMailAccountSettings* account);
			void				_RemoveAccount(BMailAccountSettings* account);
			void				_RemoveAccountFromListView(
									BMailAccountSettings* account);
			void				_AccountSelected(AccountItem* item);
			void				_ReplaceConfigView(BView* view);

private:
			BListView*			fAccountsListView;
			BMailAccountSettings* fLastSelectedAccount;
			BView*				fConfigView;
			BButton*			fRemoveButton;

			BCheckBox*			fCheckMailCheckBox;
			BTextControl*		fIntervalControl;
			BMenuField*			fStatusModeField;
			BTextView*			fHowToTextView;

			bool				fSaveSettings;
			BObjectList<BMailAccountSettings>	fAccounts;
			BObjectList<BMailAccountSettings>	fToDeleteAccounts;
};

#endif	/* CONFIG_WINDOW_H */
