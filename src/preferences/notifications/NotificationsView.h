/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APPS_VIEW_H
#define _APPS_VIEW_H

#include <View.h>

#include <notification/AppUsage.h>

typedef std::map<BString, AppUsage *> appusage_t;

class BCheckBox;
class BTextControl;
class BColumnListView;
class BStringColumn;
class BDateColumn;

class NotificationsView : public BView {
public:
								NotificationsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* msg);

private:
			status_t			_LoadAppUsage();
			void				_PopulateApplications();
			void				_Populate(AppUsage* usage);

			appusage_t			fAppFilters;
			BCheckBox*			fBlockAll;
			BTextControl*		fSearch;
			BColumnListView*	fApplications;
			BStringColumn*		fAppCol;
			BStringColumn*		fAppEnabledCol;
			BColumnListView*	fNotifications;
			BStringColumn*		fTitleCol;
			BDateColumn*		fDateCol;
			BStringColumn*		fTypeCol;
			BStringColumn*		fAllowCol;
};

#endif // _APPS_VIEW_H
