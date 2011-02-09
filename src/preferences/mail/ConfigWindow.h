#ifndef CONFIG_WINDOW_H
#define CONFIG_WINDOW_H
/* ConfigWindow - main eMail config window
 *
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
*/


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
class CenterContainer;


enum item_types
{
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
			BMailAccountSettings*	GetAccount() { return fAccount; }
			item_types			GetType() { return fType; }

			void				SetConfigPanel(BView* panel);
			BView*				ConfigPanel();
private:
			BMailAccountSettings*	fAccount;
			item_types			fType;
			BView*				fConfigPanel;
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
			void				_MakeHowToView();

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

			BListView*			fAccountsListView;
			BMailAccountSettings*	fLastSelectedAccount;
			CenterContainer*	fConfigView;
			BButton*			fRemoveButton;

			BTextControl*		fIntervalControl;
			BMenuField*			fIntervalUnitField;
			BCheckBox*			fPPPActiveCheckBox;
			BCheckBox*			fPPPActiveSendCheckBox;
			BMenuField*			fStatusModeField;
			BCheckBox*			fAutoStartCheckBox;

			bool				fSaveSettings;
			BObjectList<BMailAccountSettings>	fAccounts;
			BObjectList<BMailAccountSettings>	fToDeleteAccounts;
};

#endif	/* CONFIG_WINDOW_H */
