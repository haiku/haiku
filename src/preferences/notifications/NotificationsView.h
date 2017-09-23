/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APPS_VIEW_H
#define _APPS_VIEW_H

#include <FilePanel.h>
#include <ColumnListView.h>
#include <View.h>

#include <notification/AppUsage.h>

#include "AppRefFilter.h"
#include "SettingsPane.h"

typedef std::map<BString, AppUsage *> appusage_t;

class BButton;
class BCheckBox;
class BTextControl;
class BStringColumn;
class BDateColumn;


class AppRow : public BRow {
public:
								AppRow(const char* name,
									const char* signature, bool allowed);

			const char*			Name() const { return fName.String(); }
			const char*			Signature() { return fSignature.String(); };
			void				SetAllowed(bool allowed);
			bool				Allowed() { return fAllowed; };
			void				RefreshEnabledField();

private:
			BString				fName;
			BString				fSignature;
			bool				fAllowed;
};


class NotificationsView : public SettingsPane {
public:
								NotificationsView(SettingsHost* host);
								~NotificationsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* msg);
			status_t			Revert();
			bool				RevertPossible();
			status_t			Defaults();
			bool				DefaultsPossible();
			bool				UseDefaultRevertButtons();

private:
			status_t			Load(BMessage&);
			status_t			Save(BMessage&);
			void				_ClearItemSettings();
			void				_UpdateSelectedItem();
			void				_RecallItemSettings();
			void				_PopulateApplications();

			appusage_t			fAppFilters;
			AppRefFilter*		fPanelFilter;
			BFilePanel*			fAddAppPanel;
			BButton*			fAddButton;
			BButton*			fRemoveButton;
			BCheckBox*			fMuteAll;
			BColumnListView*	fApplications;
			AppRow*				fSelectedRow;
			BStringColumn*		fAppCol;
			BStringColumn*		fAppEnabledCol;
};

#endif // _APPS_VIEW_H
