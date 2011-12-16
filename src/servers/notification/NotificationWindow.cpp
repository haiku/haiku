/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */


#include "NotificationWindow.h"

#include <algorithm>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Layout.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <private/interface/WindowPrivate.h>

#include "AppGroupView.h"
#include "AppUsage.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "NotificationWindow"


property_info main_prop_list[] = {
	{"message", {B_GET_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}, 
		"get a message"},
	{"message", {B_COUNT_PROPERTIES, 0}, {B_DIRECT_SPECIFIER, 0},
		"count messages"},
	{"message", {B_CREATE_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
		"create a message"},
	{"message", {B_SET_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0},
		"modify a message"},
	{0}
};


const float kCloseSize				= 8;
const float kExpandSize				= 8;
const float kPenSize				= 1;
const float kEdgePadding			= 2;
const float kSmallPadding			= 2;

NotificationWindow::NotificationWindow()
	:
	BWindow(BRect(0, 0, -1, -1), B_TRANSLATE_MARK("Notification"), 
		B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_AVOID_FRONT
		| B_AVOID_FOCUS | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE | B_NOT_MOVABLE | B_AUTO_UPDATE_SIZE_LIMITS, 
		B_ALL_WORKSPACES)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0));

	_LoadSettings(true);
	_LoadAppFilters(true);

	// Start the message loop
	Hide();
	Show();
}


NotificationWindow::~NotificationWindow()
{
	appfilter_t::iterator aIt;
	for (aIt = fAppFilters.begin(); aIt != fAppFilters.end(); aIt++)
		delete aIt->second;
}


bool
NotificationWindow::QuitRequested()
{
	appview_t::iterator aIt;
	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		aIt->second->RemoveSelf();
		delete aIt->second;
	}

	BMessenger(be_app).SendMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}


void
NotificationWindow::WorkspaceActivated(int32 /*workspace*/, bool active)
{
	// Ensure window is in the correct position
	if (active)
		_ResizeAll();
}


void
NotificationWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			_LoadSettings();
			_LoadAppFilters();
			break;
		}
		case B_COUNT_PROPERTIES:
		{
			BMessage reply(B_REPLY);
			BMessage specifier;
			const char* property = NULL;
			bool messageOkay = true;

			if (message->FindMessage("specifiers", 0, &specifier) != B_OK)
				messageOkay = false;
			if (specifier.FindString("property", &property) != B_OK)
				messageOkay = false;
			if (strcmp(property, "message") != 0)
				messageOkay = false;

			if (messageOkay)
				reply.AddInt32("result", fViews.size());
			else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			}

			message->SendReply(&reply);
			break;
		}
		case B_CREATE_PROPERTY:
		case kNotificationMessage:
		{
			BMessage reply(B_REPLY);
			BNotification* notification = new BNotification(message);

			if (notification->InitCheck() == B_OK) {
				bigtime_t timeout;
				if (message->FindInt64("timeout", &timeout) != B_OK)
					timeout = -1;
				BMessenger messenger = message->ReturnAddress();
				app_info info;

				if (messenger.IsValid())
					be_roster->GetRunningAppInfo(messenger.Team(), &info);
				else
					be_roster->GetAppInfo("application/x-vnd.Be-SHEL", &info);

				NotificationView* view = new NotificationView(this,
					notification, timeout);

				bool allow = false;
				appfilter_t::iterator it = fAppFilters.find(info.signature);

				if (it == fAppFilters.end()) {
					AppUsage* appUsage = new AppUsage(notification->Group(),
						true);

					appUsage->Allowed(notification->Title(),
							notification->Type());
					fAppFilters[info.signature] = appUsage;
					allow = true;
				} else {
					allow = it->second->Allowed(notification->Title(),
						notification->Type());
				}

				if (allow) {
					BString groupName(notification->Group());
					appview_t::iterator aIt = fAppViews.find(groupName);
					AppGroupView* group = NULL;
					if (aIt == fAppViews.end()) {
						group = new AppGroupView(this,
							groupName == "" ? NULL : groupName.String());
						fAppViews[groupName] = group;
						GetLayout()->AddView(group);
					} else
						group = aIt->second;

					group->AddInfo(view);

					_ResizeAll();

					reply.AddInt32("error", B_OK);
				} else
					reply.AddInt32("error", B_NOT_ALLOWED);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			}

			message->SendReply(&reply);
			break;
		}
		case kRemoveView:
		{
			NotificationView* view = NULL;
			if (message->FindPointer("view", (void**)&view) != B_OK)
				return;

			views_t::iterator it = find(fViews.begin(), fViews.end(), view);

			if (it != fViews.end())
				fViews.erase(it);

			_ResizeAll();
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}


BHandler*
NotificationWindow::ResolveSpecifier(BMessage* msg, int32 index,
	BMessage* spec, int32 form, const char* prop)
{
	BPropertyInfo prop_info(main_prop_list);
	BHandler* handler = NULL;

	if (strcmp(prop,"message") == 0) {
		switch (msg->what) {
			case B_CREATE_PROPERTY:
			{
				msg->PopSpecifier();
				handler = this;
				break;
			}
			case B_SET_PROPERTY:
			case B_GET_PROPERTY:
			{
				int32 i;

				if (spec->FindInt32("index", &i) != B_OK)
					i = -1;

				if (i >= 0 && i < (int32)fViews.size()) {
					msg->PopSpecifier();
					handler = fViews[i];
				} else
					handler = NULL;
				break;
			}
			case B_COUNT_PROPERTIES:
				msg->PopSpecifier();
				handler = this;
				break;
			default:
				break;
		}
	}

	if (!handler)
		handler = BWindow::ResolveSpecifier(msg, index, spec, form, prop);

	return handler;
}


icon_size
NotificationWindow::IconSize()
{
	return fIconSize;
}


int32
NotificationWindow::Timeout()
{
	return fTimeout;
}


float
NotificationWindow::Width()
{
	return fWidth;
}


void
NotificationWindow::_ResizeAll()
{
	appview_t::iterator aIt;
	bool shouldHide = true;

	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		AppGroupView* app = aIt->second;
		if (app->HasChildren()) {
			shouldHide = false;
			break;
		}
	}

	if (shouldHide) {
		if (!IsHidden())
			Hide();
		return;
	}

	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		AppGroupView* view = aIt->second;

		if (!view->HasChildren()) {
			if (!view->IsHidden())
				view->Hide();
		} else {
			if (view->IsHidden())
				view->Show();
		}
	}

	SetPosition();

	if (IsHidden())
		Show();
}


void
NotificationWindow::SetPosition()
{
	Layout(true);

	BRect bounds = DecoratorFrame();
	float width = Bounds().Width() + 1;
	float height = Bounds().Height() + 1;

	float leftOffset = Frame().left - bounds.left;
	float topOffset = Frame().top - bounds.top + 1;
	float rightOffset = bounds.right - Frame().right;
	float bottomOffset = bounds.bottom - Frame().bottom;
		// Size of the borders around the window
	
	float x = Frame().left, y = Frame().top;
		// If we can't guess, don't move...

	BDeskbar deskbar;
	BRect frame = deskbar.Frame();

	switch (deskbar.Location()) {
		case B_DESKBAR_TOP:
			// Put it just under, top right corner
			y = frame.bottom + topOffset;
			x = frame.right - width + rightOffset;
			break;
		case B_DESKBAR_BOTTOM:
			// Put it just above, lower left corner
			y = frame.top - height - bottomOffset;
			x = frame.right - width + rightOffset;
			break;
		case B_DESKBAR_RIGHT_TOP:
			x = frame.left - width - rightOffset;
			y = frame.top - topOffset;
			break;
		case B_DESKBAR_LEFT_TOP:
			x = frame.right + leftOffset;
			y = frame.top - topOffset;
			break;
		case B_DESKBAR_RIGHT_BOTTOM:
			y = frame.bottom - height + bottomOffset;
			x = frame.left - width - rightOffset;
			break;
		case B_DESKBAR_LEFT_BOTTOM:
			y = frame.bottom - height + bottomOffset;
			x = frame.right + leftOffset;
			break;
		default:
			break;
	}

	MoveTo(x, y);
}


void
NotificationWindow::_LoadSettings(bool startMonitor)
{
	_LoadGeneralSettings(startMonitor);
	_LoadDisplaySettings(startMonitor);
}


void
NotificationWindow::_LoadAppFilters(bool startMonitor)
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsDirectory);

	if (create_directory(path.Path(), 0755) != B_OK)
		return;

	path.Append(kFiltersSettings);

	BFile file(path.Path(), B_READ_ONLY);
	BMessage settings;
	if (settings.Unflatten(&file) != B_OK)
		return;

	type_code type;
	int32 count = 0;

	if (settings.GetInfo("app_usage", &type, &count) != B_OK)
		return;

	for (int32 i = 0; i < count; i++) {
		AppUsage* app = new AppUsage();
		settings.FindFlat("app_usage", i, app);
		fAppFilters[app->Name()] = app;
	}

	if (startMonitor) {
		node_ref nref;
		BEntry entry(path.Path());
		entry.GetNodeRef(&nref);

		if (watch_node(&nref, B_WATCH_ALL, BMessenger(this)) != B_OK) {
			BAlert* alert = new BAlert(B_TRANSLATE("Warning"),
					B_TRANSLATE("Couldn't start filter monitor."
						" Live filter changes disabled."), B_TRANSLATE("Darn."));
			alert->Go();
		}
	}
}


void
NotificationWindow::_SaveAppFilters()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsDirectory);
	path.Append(kFiltersSettings);

	BMessage settings;
	BFile file(path.Path(), B_WRITE_ONLY);

	appfilter_t::iterator fIt;
	for (fIt = fAppFilters.begin(); fIt != fAppFilters.end(); fIt++)
		settings.AddFlat("app_usage", fIt->second);

	settings.Flatten(&file);
}


void
NotificationWindow::_LoadGeneralSettings(bool startMonitor)
{
	BPath path;
	BMessage settings;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsDirectory);
	if (create_directory(path.Path(), 0755) == B_OK) {
		path.Append(kGeneralSettings);

		BFile file(path.Path(), B_READ_ONLY);
		settings.Unflatten(&file);
	}

	if (settings.FindInt32(kTimeoutName, &fTimeout) != B_OK)
		fTimeout = kDefaultTimeout;

	// Notify the view about the change
	views_t::iterator it;
	for (it = fViews.begin(); it != fViews.end(); ++it) {
		NotificationView* view = (*it);
		view->Invalidate();
	}

	if (startMonitor) {
		node_ref nref;
		BEntry entry(path.Path());
		entry.GetNodeRef(&nref);

		if (watch_node(&nref, B_WATCH_ALL, BMessenger(this)) != B_OK) {
			BAlert* alert = new BAlert(B_TRANSLATE("Warning"),
						B_TRANSLATE("Couldn't start general settings monitor.\n"
						"Live filter changes disabled."), B_TRANSLATE("OK"));
			alert->Go();
		}
	}
}


void
NotificationWindow::_LoadDisplaySettings(bool startMonitor)
{
	BPath path;
	BMessage settings;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsDirectory);
	if (create_directory(path.Path(), 0755) == B_OK) {
		path.Append(kDisplaySettings);

		BFile file(path.Path(), B_READ_ONLY);
		settings.Unflatten(&file);
	}

	int32 setting;

	if (settings.FindFloat(kWidthName, &fWidth) != B_OK)
		fWidth = kDefaultWidth;

	if (settings.FindInt32(kIconSizeName, &setting) != B_OK)
		fIconSize = kDefaultIconSize;
	else
		fIconSize = (icon_size)setting;

	// Notify the view about the change
	views_t::iterator it;
	for (it = fViews.begin(); it != fViews.end(); ++it) {
		NotificationView* view = (*it);
		view->Invalidate();
	}

	if (startMonitor) {
		node_ref nref;
		BEntry entry(path.Path());
		entry.GetNodeRef(&nref);

		if (watch_node(&nref, B_WATCH_ALL, BMessenger(this)) != B_OK) {
			BAlert* alert = new BAlert(B_TRANSLATE("Warning"),
				B_TRANSLATE("Couldn't start display settings monitor.\n"
					"Live filter changes disabled."), B_TRANSLATE("OK"));
			alert->Go();
		}
	}
}
