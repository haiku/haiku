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
#include <Debug.h>
#include <File.h>
#include <NodeMonitor.h>
#include <PropertyInfo.h>
#include <private/interface/WindowPrivate.h>

#include "AppGroupView.h"
#include "AppUsage.h"
#include "BorderView.h"

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
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_MARK("Notification"), 
		B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_AVOID_FRONT
		| B_AVOID_FOCUS | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE | B_NOT_MOVABLE, 
		B_ALL_WORKSPACES)
{
	fBorder = new BorderView(Bounds(), "Notification");

	AddChild(fBorder);
	
	// Needed so everything gets the right size - we should switch to layout
	// mode...
	Show();
	Hide();

	LoadSettings(true);
	LoadAppFilters(true);
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
		ResizeAll();
}


void
NotificationWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			LoadSettings();
			LoadAppFilters();
			break;
		}
		case kResizeToFit:
			ResizeAll();
			break;
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
			int32 type;
			const char* content = NULL;
			const char* title = NULL;
			const char* app = NULL;
			BMessage reply(B_REPLY);
			bool messageOkay = true;

			if (message->FindInt32("type", &type) != B_OK)
				type = B_INFORMATION_NOTIFICATION;
			if (message->FindString("content", &content) != B_OK)
				messageOkay = false;
			if (message->FindString("title", &title) != B_OK)
				messageOkay = false;
			if (message->FindString("app", &app) != B_OK
				&& message->FindString("appTitle", &app) != B_OK)
				messageOkay = false;

			if (messageOkay) {
				NotificationView* view = new NotificationView(this,
					(notification_type)type, app, title, content,
					new BMessage(*message));

				appfilter_t::iterator fIt = fAppFilters.find(app);
				bool allow = false;
				if (fIt == fAppFilters.end()) {
					app_info info;
					BMessenger messenger = message->ReturnAddress();
					if (messenger.IsValid())
						be_roster->GetRunningAppInfo(messenger.Team(), &info);
					else
						be_roster->GetAppInfo("application/x-vnd.Be-SHEL", &info);

					AppUsage* appUsage = new AppUsage(info.ref, app, true);
					fAppFilters[app] = appUsage;

					appUsage->Allowed(title, (notification_type)type);

					allow = true;
				} else
					allow = fIt->second->Allowed(title, (notification_type)type);

				if (allow) {
					appview_t::iterator aIt = fAppViews.find(app);
					AppGroupView* group = NULL;
					if (aIt == fAppViews.end()) {
						group = new AppGroupView(this, app);
						fAppViews[app] = group;
						fBorder->AddChild(group);
					} else
						group = aIt->second;

					group->AddInfo(view);

					ResizeAll();

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
			void* _ptr;
			message->FindPointer("view", &_ptr);

			NotificationView* info
				= reinterpret_cast<NotificationView*>(_ptr);

			fBorder->RemoveChild(info);

			std::vector<NotificationView*>::iterator i
				= find(fViews.begin(), fViews.end(), info);
			if (i != fViews.end())
				fViews.erase(i);

			delete info;

			ResizeAll();
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


infoview_layout
NotificationWindow::Layout()
{
	return fLayout;
}


float
NotificationWindow::ViewWidth()
{
	return fWidth;
}


void
NotificationWindow::ResizeAll()
{
	if (fAppViews.empty()) {
		if (!IsHidden())
			Hide();
		return;
	}

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

	if (IsHidden())
		Show();

	float width = 0;
	float height = 0;

	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		AppGroupView* view = aIt->second;
		float w = -1;
		float h = -1;

		if (!view->HasChildren()) {
			if (!view->IsHidden())
				view->Hide();
		} else {
			view->GetPreferredSize(&w, &h);
			width = max_c(width, h);

			view->ResizeToPreferred();
			view->MoveTo(0, height);

			height += h;

			if (view->IsHidden())
				view->Show();
		}
	}

	ResizeTo(ViewWidth(), height);
	PopupAnimation();
}


void
NotificationWindow::SetPosition()
{
	BRect bounds = DecoratorFrame();
	float width = Bounds().Width() + 1;
	float height = Bounds().Height() + 1;
	
	float leftOffset = Frame().left - bounds.left;
	float topOffset = Frame().top - bounds.top;
	float rightOffset = bounds.right - Frame().right;
	float bottomOffset = bounds.bottom - Frame().bottom;
		// Size of the borders around the window
	printf("%f %f %f %f\n",leftOffset, topOffset, rightOffset, bottomOffset);
	
	float x = Frame().left, y = Frame().top;
		// If we can't guess, don't move...

	BDeskbar deskbar;
	BRect frame = deskbar.Frame();

	switch (deskbar.Location()) {
		case B_DESKBAR_TOP:
			// Put it just under, top right corner
			y = frame.bottom + topOffset;
			x = frame.right - width - rightOffset;
			break;
		case B_DESKBAR_BOTTOM:
			// Put it just above, lower left corner
			y = frame.top - height - bottomOffset;
			x = frame.right - width - rightOffset;
			break;
		case B_DESKBAR_RIGHT_TOP:
			x = frame.left - width - rightOffset;
			y = frame.top + topOffset;
			break;
		case B_DESKBAR_LEFT_TOP:
			x = frame.right + leftOffset;
			y = frame.top + topOffset;
			break;
		case B_DESKBAR_RIGHT_BOTTOM:
			y = frame.bottom - height - bottomOffset;
			x = frame.left - width - rightOffset;
			break;
		case B_DESKBAR_LEFT_BOTTOM:
			y = frame.bottom - height - bottomOffset;
			x = frame.right + leftOffset;
			break;
		default:
			break;
	}

	MoveTo(x, y);
}

void
NotificationWindow::PopupAnimation()
{
	SetPosition();

	if (IsHidden() && fViews.size() != 0)
		Show();
	// Activate();// it hides floaters from apps :-(
}


void
NotificationWindow::LoadSettings(bool startMonitor)
{
	_LoadGeneralSettings(startMonitor);
	_LoadDisplaySettings(startMonitor);
}


void
NotificationWindow::LoadAppFilters(bool startMonitor)
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
NotificationWindow::SaveAppFilters()
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


void NotificationWindow::Show()
{
	BWindow::Show();
	SetPosition();
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
		view->SetText(view->Application(), view->Title(), view->Text());
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

	if (settings.FindInt32(kLayoutName, &setting) != B_OK)
		fLayout = kDefaultLayout;
	else {
		switch (setting) {
			case 0:
				fLayout = TitleAboveIcon;
				break;
			case 1:
				fLayout = AllTextRightOfIcon;
				break;
			default:
				fLayout = kDefaultLayout;
		}
	}

	// Notify the view about the change
	views_t::iterator it;
	for (it = fViews.begin(); it != fViews.end(); ++it) {
		NotificationView* view = (*it);
		view->SetText(view->Application(), view->Title(), view->Text());
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
