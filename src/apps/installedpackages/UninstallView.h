/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef UNINSTALLVIEW_H
#define UNINSTALLVIEW_H

#include <View.h>
#include <Layout.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Button.h>
#include <TextView.h>
#include <Path.h>

#include "InstalledPackageInfo.h"


class UninstallView : public BView {
public:
							UninstallView(BRect frame);
	virtual					~UninstallView();

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);
		
private:
	class InfoItem;

			void			_InitView();
			status_t		_ReloadAppList();
			void			_AddFile(const char* filename, const node_ref& ref);
			void			_ClearAppList();
			void			_CachePathToPackages();

			BPath			fToPackages;
			BListView*		fAppList;
			BTextView*		fDescription;
			BButton*		fButton;
			BScrollView*	fDescScroll;
			InstalledPackageInfo fCurrentSelection;
			bool			fWatcherRunning;
};

#endif // UNINSTALLVIEW_H
