/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Brian Hill, supernova@tycho.email
 */

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Notification.h>
#include <Path.h>
#include <TextControl.h>
#include <Window.h>

#include <notification/Notifications.h>
#include <notification/NotificationReceived.h>

#include "NotificationsConstants.h"
#include "NotificationsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NotificationView"

// Applications column indexes
const int32 kAppNameIndex = 0;
const int32 kAppEnabledIndex = 1;


AppRow::AppRow(const char* name, const char* signature, bool allowed)
	:
	BRow(),
	fName(name),
	fSignature(signature),
	fAllowed(allowed)
{
	SetField(new BStringField(fName.String()), kAppNameIndex);
	BString text = fAllowed ? B_TRANSLATE("Allowed") : B_TRANSLATE("Muted");
	SetField(new BStringField(text.String()), kAppEnabledIndex);
}


void
AppRow::SetAllowed(bool allowed)
{
	fAllowed = allowed;
	RefreshEnabledField();
}


void
AppRow::RefreshEnabledField()
{
	BStringField* field = (BStringField*)GetField(kAppEnabledIndex);
	BString text = fAllowed ? B_TRANSLATE("Allowed") : B_TRANSLATE("Muted");
	field->SetString(text.String());
	Invalidate();
}


NotificationsView::NotificationsView(SettingsHost* host)
	:
	SettingsPane("apps", host),
	fSelectedRow(NULL)
{
	// Applications list
	fApplications = new BColumnListView(B_TRANSLATE("Applications"),
		0, B_FANCY_BORDER, false);
	fApplications->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fApplications->SetSelectionMessage(new BMessage(kApplicationSelected));

	float colWidth = be_plain_font->StringWidth(B_TRANSLATE("Application"))
		+ (kCLVTitlePadding * 2);
	fAppCol = new BStringColumn(B_TRANSLATE("Application"), colWidth * 2,
		colWidth, colWidth * 4, B_TRUNCATE_END, B_ALIGN_LEFT);
	fApplications->AddColumn(fAppCol, kAppNameIndex);

	colWidth = be_plain_font->StringWidth(B_TRANSLATE("Status"))
		+ (kCLVTitlePadding * 2);
	fAppEnabledCol = new BStringColumn(B_TRANSLATE("Status"), colWidth * 1.5,
		colWidth, colWidth * 3, B_TRUNCATE_END, B_ALIGN_LEFT);
	fApplications->AddColumn(fAppEnabledCol, kAppEnabledIndex);
	fApplications->SetSortColumn(fAppCol, true, true);
	
	fAddButton = new BButton("add_app", B_TRANSLATE("Add" B_UTF8_ELLIPSIS),
		new BMessage(kAddApplication));
	fRemoveButton = new BButton("add_app", B_TRANSLATE("Remove"),
		new BMessage(kRemoveApplication));
	fRemoveButton->SetEnabled(false);
	
	fMuteAll = new BCheckBox("block", B_TRANSLATE("Mute notifications from "
		"this application"),
		new BMessage(kMuteChanged));

	// Add views
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(fApplications)
			.AddGroup(B_VERTICAL)
				.Add(fAddButton)
				.Add(fRemoveButton)
				.AddGlue()
			.End()
		.End()
		.Add(fMuteAll)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING);

	// Set button sizes
	float maxButtonWidth = std::max(fAddButton->PreferredSize().Width(),
		fRemoveButton->PreferredSize().Width());
	fAddButton->SetExplicitMaxSize(BSize(maxButtonWidth, B_SIZE_UNSET));
	fRemoveButton->SetExplicitMaxSize(BSize(maxButtonWidth, B_SIZE_UNSET));

	// File Panel
	fPanelFilter = new AppRefFilter();
	fAddAppPanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, false,
		NULL, fPanelFilter);
}


NotificationsView::~NotificationsView()
{
	delete fAddAppPanel;
	delete fPanelFilter;
}


void
NotificationsView::AttachedToWindow()
{
	fApplications->SetTarget(this);
	fApplications->SetInvocationMessage(new BMessage(kApplicationSelected));
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	fMuteAll->SetTarget(this);
	fAddAppPanel->SetTarget(this);
	_RecallItemSettings();
}


void
NotificationsView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kApplicationSelected:
		{
			Window()->Lock();
			_ClearItemSettings();
			_UpdateSelectedItem();
			_RecallItemSettings();
			Window()->Unlock();
			break;
		}
		case kMuteChanged:
		{
			bool allowed = fMuteAll->Value() == B_CONTROL_OFF;
			fSelectedRow->SetAllowed(allowed);
			appusage_t::iterator it = fAppFilters.find(fSelectedRow->Signature());
			if (it != fAppFilters.end())
				it->second->SetAllowed(allowed);
			Window()->PostMessage(kApply);
			break;
		}
		case kAddApplication:
		{
			BMessage addmsg(kAddApplicationRef);
			fAddAppPanel->SetMessage(&addmsg);
			fAddAppPanel->Show();
			break;
		}
		case kAddApplicationRef:
		{
			entry_ref srcRef;
			msg->FindRef("refs", &srcRef);
			BEntry srcEntry(&srcRef, true);
			BPath path(&srcEntry);
			BNode node(&srcEntry);
			char *buf = new char[B_ATTR_NAME_LENGTH];
			ssize_t size;
			if ( (size = node.ReadAttr("BEOS:APP_SIG", 0, 0, buf,
				B_ATTR_NAME_LENGTH)) > 0 )
			{
				// Search for already existing app
				appusage_t::iterator it = fAppFilters.find(buf);
				if (it != fAppFilters.end()) {
					BString text(path.Leaf());
					text.Append(B_TRANSLATE_COMMENT(" is already listed",
							"Alert message"));
					BAlert* alert = new BAlert("", text.String(),
						B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT);
					alert->Go(NULL);
				} else {
					AppUsage* appUsage = new AppUsage(path.Leaf(), buf, true);
					fAppFilters[appUsage->Signature()] = appUsage;
					AppRow* row = new AppRow(appUsage->AppName(),
						appUsage->Signature(), appUsage->Allowed());
					fApplications->AddRow(row);
					fApplications->DeselectAll();
					fApplications->AddToSelection(row);
					fApplications->ScrollTo(row);
					_UpdateSelectedItem();
					_RecallItemSettings();
					//row->Invalidate();
					//fApplications->InvalidateRow(row);
					// TODO redraw row properly
					Window()->PostMessage(kApply);
				}
			} else {
				BAlert* alert = new BAlert("",
					B_TRANSLATE_COMMENT("Application does not have "
						"a valid signature", "Alert message"),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
					B_WARNING_ALERT);
				alert->Go(NULL);
			}
			delete[] buf;
			break;
		}
		case kRemoveApplication:
		{
			if (fSelectedRow) {
				appusage_t::iterator it = fAppFilters.find(fSelectedRow->Signature());
				if (it != fAppFilters.end()) {
					delete it->second;
					fAppFilters.erase(it);
				}
				fApplications->RemoveRow(fSelectedRow);
				delete fSelectedRow;
				fSelectedRow = NULL;
				_ClearItemSettings();
				_UpdateSelectedItem();
				_RecallItemSettings();
				Window()->PostMessage(kApply);
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


status_t
NotificationsView::Load(BMessage& settings)
{
	type_code type;
	int32 count = 0;

	if (settings.GetInfo("app_usage", &type, &count) != B_OK)
		return B_ERROR;

	// Clean filters
	appusage_t::iterator auIt;
	for (auIt = fAppFilters.begin(); auIt != fAppFilters.end(); auIt++)
		delete auIt->second;
	fAppFilters.clear();

	// Add new filters
	for (int32 i = 0; i < count; i++) {
		AppUsage* app = new AppUsage();
		settings.FindFlat("app_usage", i, app);
		fAppFilters[app->Signature()] = app;
	}

	// Load the applications list
	_PopulateApplications();

	return B_OK;
}


status_t
NotificationsView::Save(BMessage& storage)
{
	appusage_t::iterator fIt;
	for (fIt = fAppFilters.begin(); fIt != fAppFilters.end(); fIt++)
		storage.AddFlat("app_usage", fIt->second);

	return B_OK;
}


void
NotificationsView::_ClearItemSettings()
{
	fMuteAll->SetValue(B_CONTROL_OFF);
}


void
NotificationsView::_UpdateSelectedItem()
{
	fSelectedRow = dynamic_cast<AppRow*>(fApplications->CurrentSelection());
	
}


void
NotificationsView::_RecallItemSettings()
{
	// No selected item
	if(fSelectedRow == NULL)
	{
		fMuteAll->SetValue(B_CONTROL_OFF);
		fMuteAll->SetEnabled(false);
		fRemoveButton->SetEnabled(false);
	} else {
		fMuteAll->SetEnabled(true);
		fRemoveButton->SetEnabled(true);
		appusage_t::iterator it = fAppFilters.find(fSelectedRow->Signature());
		if (it != fAppFilters.end())
			fMuteAll->SetValue(!(it->second->Allowed()));
	}
}


status_t
NotificationsView::Revert()
{
	return B_OK;
}


bool
NotificationsView::RevertPossible()
{
	return false;
}


status_t
NotificationsView::Defaults()
{
	return B_OK;
}


bool
NotificationsView::DefaultsPossible()
{
	return false;
}


bool
NotificationsView::UseDefaultRevertButtons()
{
	return false;
}


void
NotificationsView::_PopulateApplications()
{
	fApplications->Clear();

	appusage_t::iterator it;
	for (it = fAppFilters.begin(); it != fAppFilters.end(); ++it) {
		AppUsage* appUsage = it->second;
		AppRow* row = new AppRow(appUsage->AppName(),
			appUsage->Signature(), appUsage->Allowed());
		fApplications->AddRow(row);
	}
}
