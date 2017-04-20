/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef _SOFTWARE_UPDATER_WINDOW_H
#define _SOFTWARE_UPDATER_WINDOW_H


#include <Button.h>
#include <GroupView.h>
#include <OutlineListView.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Window.h>

#include "StripeView.h"

using namespace BPrivate;

enum {
	PACKAGE_UPDATE,
	PACKAGE_INSTALL,
	PACKAGE_UNINSTALL
};


class SuperItem : public BListItem {
public:
								SuperItem(const char* label);
								~SuperItem();
	virtual void				DrawItem(BView*, BRect, bool);
	virtual void				Update(BView *owner, const BFont *font);
	font_height					GetFontHeight() { return fFontHeight; };
	float						GetPackageItemHeight()
									{ return fPackageItemHeight; };
	BBitmap*					GetIcon() { return fPackageIcon; };
	int16						GetIconSize() { return fIconSize; };
	void						SetDetailLevel(bool showMoreDetails);
	bool						GetDetailLevel() { return fShowMoreDetails; };
	void						SetItemCount(int32 count)
									{ fItemCount = count; };

private:
			void				_SetHeights();
			void				_GetPackageIcon();
			
			BString				fLabel;
			BFont				fRegularFont;
			BFont				fBoldFont;
			bool				fShowMoreDetails;
			font_height			fFontHeight;
			float				fPackageItemHeight;
			BBitmap*			fPackageIcon;
			int16				fIconSize;
			int32				fItemCount;
};


class PackageItem : public BListItem {
public:
								PackageItem(const char* name,
									const char* simple_version,
									const char* detailed_version,
									const char* repository,
									const char* summary,
									const char* file_name,
									SuperItem* super);
	virtual void				DrawItem(BView*, BRect, bool);
	virtual void				Update(BView *owner, const BFont *font);
	void						SetItemHeight(const BFont* font);
	int							NameCompare(PackageItem* item);
	const char*					FileName() { return fFileName.String(); };
	void						SetDownloadProgress(float percent);
	void						ShowProgressBar() { fDrawBarFlag = true; };
	
private:
			void				_DrawBar(BPoint where, BView* view,
									icon_size which);

			BString				fName;
			BString				fSimpleVersion;
			BString				fDetailedVersion;
			BString				fRepository;
			BString				fSummary;
			BFont				fRegularFont;
			BFont				fSmallFont;
			font_height			fSmallFontHeight;
			float				fSmallTotalHeight;
			float				fLabelOffset;
			SuperItem*			fSuperItem;
			BString				fFileName;
			float				fDownloadProgress;
			bool				fDrawBarFlag;
};


class PackageListView : public BOutlineListView {
public:
								PackageListView();
			void				AttachedToWindow();
	virtual void				MessageReceived(BMessage*);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual void				MouseDown(BPoint where);
			void				AddPackage(uint32 install_type,
									const char* name,
									const char* cur_ver,
									const char* new_ver,
									const char* summary,
									const char* repository,
									const char* file_name);
			void				UpdatePackageProgress(const char* packageName,
									float percent);
			void				SortItems();
			float				ItemHeight();

private:
			void				_SetItemHeights();

			SuperItem*			fSuperUpdateItem;
			SuperItem*			fSuperInstallItem;
			SuperItem*			fSuperUninstallItem;
			bool				fShowMoreDetails;
			BPopUpMenu			*fMenu;
			BMenuItem			*fDetailMenuItem;
			PackageItem*		fLastProgressItem;
			int16				fLastProgressValue;
};


class SoftwareUpdaterWindow : public BWindow {
public:
								SoftwareUpdaterWindow();
			void				MessageReceived(BMessage* message);
			bool				ConfirmUpdates();
			void				UpdatesApplying(const char* header,
									const char* detail);
			bool				UserCancelRequested();
			void				AddPackageInfo(uint32 install_type,
									const char* package_name,
									const char* cur_ver,
									const char* new_ver,
									const char* summary,
									const char* repository,
									const char* file_name);
			void				ShowWarningAlert(const char* text);
			BBitmap				GetIcon(int32 iconSize);
			BBitmap				GetNotificationIcon();
			BRect				GetDefaultRect() { return fDefaultRect; };
			BPoint				GetLocation() { return Frame().LeftTop(); };
			BLayoutItem*		layout_item_for(BView* view);
			void				FinalUpdate(const char* header,
									const char* detail);

private:
			uint32				_WaitForButtonClick();
			void				_SetState(uint32 state);
			uint32				_GetState();
			
			BRect				fDefaultRect;
			StripeView*			fStripeView;
			BStringView*		fHeaderView;
			BStringView*		fDetailView;
			BButton*			fUpdateButton;
			BButton*			fCancelButton;
			BStatusBar*			fStatusBar;
			PackageListView*	fListView;
			BScrollView*		fScrollView;
			BLayoutItem*		fDetailsLayoutItem;
			BLayoutItem*		fPackagesLayoutItem;
			BLayoutItem*		fProgressLayoutItem;
			BLayoutItem*		fUpdateButtonLayoutItem;
			
			uint32				fCurrentState;
			sem_id				fWaitingSem;
			bool				fWaitingForButton;
			uint32				fButtonResult;
			bool				fUserCancelRequested;
			BInvoker			fCancelAlertResponse;
			int32				fWarningAlertCount;
			BInvoker			fWarningAlertDismissed;
			
};


int SortPackageItems(const BListItem* item1, const BListItem* item2);


#endif // _SOFTWARE_UPDATER_WINDOW_H
