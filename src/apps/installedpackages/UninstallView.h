/*
 * Copyright (c) 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef UNINSTALLVIEW_H
#define UNINSTALLVIEW_H

#include <GroupView.h>
#include <Path.h>

#include "InstalledPackageInfo.h"


class BButton;
class BListView;
class BTextView;
class BScrollView;


class UninstallView : public BGroupView {
public:
								UninstallView();
	virtual						~UninstallView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_InitView();
			status_t			_ReloadAppList();
			void				_AddFile(const char* filename,
									const node_ref& ref);
			void				_ClearAppList();
			void				_CachePathToPackages();

private:
			class InfoItem;

			BPath				fToPackages;
			BListView*			fAppList;
			BTextView*			fDescription;
			BButton*			fButton;
			BScrollView*		fDescScroll;
			InstalledPackageInfo fCurrentSelection;
			bool				fWatcherRunning;

			const char*			fNoPackageSelectedString;
};


#endif // UNINSTALLVIEW_H
