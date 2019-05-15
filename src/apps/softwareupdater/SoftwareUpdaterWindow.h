/*
 * Copyright 2016-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@tycho.email>
 *		Jacob Secunda
 */
#ifndef _SOFTWARE_UPDATER_WINDOW_H
#define _SOFTWARE_UPDATER_WINDOW_H


#include <Button.h>
#include <CheckBox.h>
#include <GroupView.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <Path.h>
#include <Point.h>
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
	float						GetPackageItemHeight();
	float						GetPackageItemHeight(bool showMoreDetails);
	BBitmap*					GetIcon(bool showMoreDetails);
	float						GetIconSize(bool showMoreDetails);
	void						SetDetailLevel(bool showMoreDetails);
	bool						GetDetailLevel() { return fShowMoreDetails; };
	void						SetItemCount(int32 count);
	float						ZoomWidth(BView *owner);

private:
			BBitmap*			_GetPackageIcon(float listItemHeight);

			BString				fLabel;
			BString				fItemText;
			BFont				fRegularFont;
			BFont				fBoldFont;
			bool				fShowMoreDetails;
			font_height			fBoldFontHeight;
			float				fPackageItemLineHeight;
			BBitmap*			fPackageLessIcon;
			BBitmap*			fPackageMoreIcon;
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
	void						CalculateZoomWidths(BView *owner);
	int							NameCompare(PackageItem* item);
	const char*					FileName() { return fFileName.String(); };
	void						SetDownloadProgress(float percent);
	void						ShowProgressBar() { fDrawBarFlag = true; };
	float						MoreDetailsWidth()
									{ return fMoreDetailsWidth; };
	float						LessDetailsWidth()
									{ return fLessDetailsWidth; };

private:
			void				_DrawBar(BPoint where, BView* view,
									icon_size which);

			BString				fName;
			BString				fSimpleVersion;
			BString				fDetailedVersion;
			BString				fRepository;
			BString				fSummary;
			BFont				fSmallFont;
			font_height			fSmallFontHeight;
			float				fSmallTotalHeight;
			float				fLabelOffset;
			SuperItem*			fSuperItem;
			BString				fFileName;
			float				fDownloadProgress;
			bool				fDrawBarFlag;
			float				fMoreDetailsWidth;
			float				fLessDetailsWidth;
};


class PackageListView : public BOutlineListView {
public:
								PackageListView();
	virtual	void				FrameResized(float newWidth, float newHeight);
			void				ExpandOrCollapse(BListItem *superItem,
									bool expand);
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
			void				SetMoreDetails(bool showMore);
			BPoint				ZoomPoint();

private:
			void				_SetItemHeights();

			SuperItem*			fSuperUpdateItem;
			SuperItem*			fSuperInstallItem;
			SuperItem*			fSuperUninstallItem;
			bool				fShowMoreDetails;
			PackageItem*		fLastProgressItem;
			int16				fLastProgressValue;
};


class SoftwareUpdaterWindow : public BWindow {
public:
								SoftwareUpdaterWindow();
			bool				QuitRequested();
			void				FrameMoved(BPoint newPosition);
			void				FrameResized(float newWidth, float newHeight);
			void				Zoom(BPoint origin, float width, float height);
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
			BRect				GetDefaultRect() { return fDefaultRect; };
			BPoint				GetLocation() { return Frame().LeftTop(); };
			BLayoutItem*		layout_item_for(BView* view);
			void				FinalUpdate(const char* header,
									const char* detail);

private:
			uint32				_WaitForButtonClick();
			void				_SetState(uint32 state);
			uint32				_GetState();
			status_t			_WriteSettings();
			status_t			_ReadSettings(BMessage& settings);

			BRect				fDefaultRect;
			BStripeView*		fStripeView;
			BStringView*		fHeaderView;
			BStringView*		fDetailView;
			BButton*			fUpdateButton;
			BButton*			fCancelButton;
			BButton*			fRebootButton;
			BStatusBar*			fStatusBar;
			PackageListView*	fListView;
			BScrollView*		fScrollView;
			BCheckBox*			fDetailsCheckbox;
			BLayoutItem*		fDetailsLayoutItem;
			BLayoutItem*		fPackagesLayoutItem;
			BLayoutItem*		fProgressLayoutItem;
			BLayoutItem*		fCancelButtonLayoutItem;
			BLayoutItem*		fUpdateButtonLayoutItem;
			BLayoutItem*		fRebootButtonLayoutItem;
			BLayoutItem*		fDetailsCheckboxLayoutItem;

			uint32				fCurrentState;
			sem_id				fWaitingSem;
			bool				fWaitingForButton;
			uint32				fButtonResult;
			bool				fUpdateConfirmed;
			bool				fUserCancelRequested;
			BInvoker			fCancelAlertResponse;
			int32				fWarningAlertCount;
			BInvoker			fWarningAlertDismissed;
			BPath				fSettingsPath;
			status_t			fSettingsReadStatus;
			BMessage			fInitialSettings;
			bool				fSaveFrameChanges;
			BMessageRunner*		fMessageRunner;
			BMessage			fFrameChangeMessage;
			float				fZoomHeightBaseline;
			float				fZoomWidthBaseline;
};


int SortPackageItems(const BListItem* item1, const BListItem* item2);


#endif // _SOFTWARE_UPDATER_WINDOW_H
