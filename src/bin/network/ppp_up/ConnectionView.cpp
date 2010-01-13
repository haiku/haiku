/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "ConnectionView.h"
#include "PPPDeskbarReplicant.h"
#include <MessageDriverSettingsUtils.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Deskbar.h>
#include <Entry.h>
#include <File.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include <PPPInterface.h>
#include <settings_tools.h>
#include <algorithm>
	// for max()

using std::max;

// GUI constants
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgCancel = 'CANC';
static const uint32 kMsgConnect = 'CONN';
static const uint32 kMsgUpdate = 'MUPD';

// labels
static const char *kLabelSavePassword = "Save password";
static const char *kLabelName = "Username: ";
static const char *kLabelPassword = "Password: ";
static const char *kLabelConnect = "Connect";
static const char *kLabelCancel = "Cancel";
static const char *kLabelAuthentication = "Authentication";

// connection status strings
static const char *kTextConnecting = "Connecting...";
static const char *kTextConnectionEstablished = "Connection established.";
static const char *kTextNotConnected = "Not connected.";
static const char *kTextDeviceUpFailed = "Failed to connect.";
static const char *kTextAuthenticating = "Authenticating...";
static const char *kTextAuthenticationFailed = "Authentication failed!";
static const char *kTextConnectionLost = "Connection lost!";


ConnectionView::ConnectionView(BRect rect, const BString& interfaceName)
	: BView(rect, "ConnectionView", B_FOLLOW_NONE, 0),
	fListener(this),
	fInterfaceName(interfaceName),
	fKeepLabel(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rect = Bounds();
	rect.InsetBy(5, 5);
	rect.bottom = rect.top
		+ 25 // space for topmost control
		+ 3 * 20 // size of controls
		+ 3 * 5; // space beween controls and bottom of box
	BBox *authenticationBox = new BBox(rect, "Authentication");
	authenticationBox->SetLabel(kLabelAuthentication);
	rect = authenticationBox->Bounds();
	rect.InsetBy(10, 20);
	rect.bottom = rect.top + 20;
	fUsername = new BTextControl(rect, "username", kLabelName, NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fPassword = new BTextControl(rect, "password", kLabelPassword, NULL, NULL);
	fPassword->TextView()->HideTyping(true);
	
	// set dividers
	float width = max(StringWidth(fUsername->Label()),
		StringWidth(fPassword->Label()));
	fUsername->SetDivider(width + 5);
	fPassword->SetDivider(width + 5);
	
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fSavePassword = new BCheckBox(rect, "SavePassword", kLabelSavePassword, NULL);
	
	authenticationBox->AddChild(fUsername);
	authenticationBox->AddChild(fPassword);
	authenticationBox->AddChild(fSavePassword);
	AddChild(authenticationBox);
	
	rect = authenticationBox->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 15;
	fAttemptView = new BStringView(rect, "AttemptView", "");
	AddChild(fAttemptView);
	
	// add status view
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fStatusView = new BStringView(rect, "StatusView", "");
	AddChild(fStatusView);
	
	// add "Connect" and "Cancel" buttons
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	rect.right = rect.left + kDefaultButtonWidth;
	fConnectButton = new BButton(rect, "ConnectButton", kLabelConnect,
		new BMessage(kMsgConnect));
	
	rect.left = rect.right + 10;
	rect.right = rect.left + kDefaultButtonWidth;
	fCancelButton = new BButton(rect, "CancelButton", kLabelCancel,
		new BMessage(kMsgCancel));
	
	AddChild(fConnectButton);
	AddChild(fCancelButton);
}


void
ConnectionView::AttachedToWindow()
{
	Reload();
	fListener.WatchManager();
	WatchInterface(fListener.Manager().InterfaceWithName(fInterfaceName.String()));
	
	Window()->SetDefaultButton(fConnectButton);
	fConnectButton->SetTarget(this);
	fCancelButton->SetTarget(this);
}


void
ConnectionView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;
		
		case kMsgConnect:
			Connect();
		break;
		
		case kMsgCancel:
			Cancel();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


// update authentication UI
void
ConnectionView::Reload()
{
	// load username and password
	BString path("ptpnet/");
	path << fInterfaceName;
	fSettings.MakeEmpty();
	ReadMessageDriverSettings(path.String(), &fSettings);
	
	fHasUsername = fHasPassword = false;
	BString username, password;
	
	BMessage parameter;
	int32 parameterIndex = 0;
	if(FindMessageParameter(PPP_USERNAME_KEY, fSettings, &parameter, &parameterIndex)
			&& parameter.FindString(MDSU_VALUES, &username) == B_OK)
		fHasUsername = true;
	
	parameterIndex = 0;
	if(FindMessageParameter(PPP_PASSWORD_KEY, fSettings, &parameter, &parameterIndex)
			&& parameter.FindString(MDSU_VALUES, &password) == B_OK)
		fHasPassword = true;
	
	fUsername->SetText(username.String());
	fPassword->SetText(password.String());
	fSavePassword->SetValue(fHasPassword);
	
	fUsername->SetEnabled(fHasUsername);
	fPassword->SetEnabled(fHasUsername);
	fSavePassword->SetEnabled(fHasUsername);
}


void
ConnectionView::Connect()
{
	PPPInterface interface(PPPManager().CreateInterfaceWithName(
		fInterfaceName.String()));
	interface.SetUsername(Username());
	interface.SetPassword(Password());
	interface.SetAskBeforeConnecting(false);
	interface.Up();
	
	// save settings
	if(fHasUsername) {
		BMessage parameter;
		int32 index = 0;
		if(FindMessageParameter(PPP_USERNAME_KEY, fSettings, &parameter, &index))
			fSettings.RemoveData(MDSU_PARAMETERS, index);
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_USERNAME_KEY);
		parameter.AddString(MDSU_VALUES, Username());
		fSettings.AddMessage(MDSU_PARAMETERS, &parameter);
		
		index = 0;
		if(FindMessageParameter(PPP_PASSWORD_KEY, fSettings, &parameter, &index))
			fSettings.RemoveData(MDSU_PARAMETERS, index);
		if(DoesSavePassword()) {
			parameter.MakeEmpty();
			parameter.AddString(MDSU_NAME, PPP_PASSWORD_KEY);
			parameter.AddString(MDSU_VALUES, Password());
			fSettings.AddMessage(MDSU_PARAMETERS, &parameter);
		}
		
		BEntry entry;
		if(interface.GetSettingsEntry(&entry) == B_OK) {
			BFile file(&entry, B_WRITE_ONLY);
			WriteMessageDriverSettings(file, fSettings);
		}
	}
	
	Reload();
}


void
ConnectionView::Cancel()
{
	PPPInterface interface(fListener.Interface());
	bool quit = false;
	ppp_interface_info_t info;
	if(interface.GetInterfaceInfo(&info) && info.info.phase < PPP_ESTABLISHMENT_PHASE)
		quit = true;
	interface.Down();
	if(quit)
		Window()->Quit();
}


// Clean up before our window quits (called by ConnectionWindow).
void
ConnectionView::CleanUp()
{
	fListener.StopWatchingInterface();
	fListener.StopWatchingManager();
}


BString
ConnectionView::AttemptString() const
{
	PPPInterface interface(fListener.Interface());
	ppp_interface_info_t info;
	if(!interface.GetInterfaceInfo(&info))
		return BString("");
	BString attempt;
	attempt << "Attempt " << info.info.connectAttempt << " of " <<
		info.info.connectRetriesLimit + 1;
	
	return attempt;
}


void
ConnectionView::HandleReportMessage(BMessage *message)
{
	ppp_interface_id id;
	if(message->FindInt32("interface", reinterpret_cast<int32*>(&id)) != B_OK
			|| (fListener.Interface() != PPP_UNDEFINED_INTERFACE_ID
				&& id != fListener.Interface()))
		return;
	
	int32 type, code;
	message->FindInt32("type", &type);
	message->FindInt32("code", &code);
	
	if(type == PPP_MANAGER_REPORT && code == PPP_REPORT_INTERFACE_CREATED) {
		PPPInterface interface(id);
		if(interface.InitCheck() != B_OK || fInterfaceName != interface.Name())
			return;
		
		WatchInterface(id);
		
		if(((fHasUsername && !fHasPassword) || fAskBeforeConnecting)
				&& Window()->IsHidden())
			Window()->Show();
	} else if(type == PPP_CONNECTION_REPORT)
		UpdateStatus(code);
	else if(type == PPP_DESTRUCTION_REPORT)
		fListener.StopWatchingInterface();
}


void
ConnectionView::UpdateStatus(int32 code)
{
	BString attemptString = AttemptString();
	fAttemptView->SetText(attemptString.String());
	
	if(code == PPP_REPORT_UP_SUCCESSFUL) {
		fStatusView->SetText(kTextConnectionEstablished);
		PPPDeskbarReplicant *item = new PPPDeskbarReplicant(fListener.Interface());
		BDeskbar().AddItem(item);
		delete item;
		Window()->Quit();
		return;
	}
	
	// maybe the status string must not be changed (codes that set fKeepLabel to false
	// should still be handled)
	if(fKeepLabel && code != PPP_REPORT_GOING_UP && code != PPP_REPORT_UP_SUCCESSFUL)
		return;
	
	if(fListener.InitCheck() != B_OK) {
		fStatusView->SetText(kTextConnectionLost);
		return;
	}
	
	// only errors should set fKeepLabel to true
	switch(code) {
		case PPP_REPORT_GOING_UP:
			fKeepLabel = false;
			fStatusView->SetText(kTextConnecting);
		break;
		
		case PPP_REPORT_DOWN_SUCCESSFUL:
			fStatusView->SetText(kTextNotConnected);
		break;
		
		case PPP_REPORT_DEVICE_UP_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextDeviceUpFailed);
		break;
		
		case PPP_REPORT_AUTHENTICATION_REQUESTED:
			fStatusView->SetText(kTextAuthenticating);
		break;
		
		case PPP_REPORT_AUTHENTICATION_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextAuthenticationFailed);
		break;
		
		case PPP_REPORT_CONNECTION_LOST:
			fKeepLabel = true;
			fStatusView->SetText(kTextConnectionLost);
		break;
	}
}


void
ConnectionView::WatchInterface(ppp_interface_id ID)
{
	fListener.WatchInterface(ID);
	
	// update status
	Reload();
	PPPInterface interface(fListener.Interface());
	ppp_interface_info_t info;
	if(!interface.GetInterfaceInfo(&info)) {
		UpdateStatus(PPP_REPORT_DOWN_SUCCESSFUL);
		fAskBeforeConnecting = false;
	} else
		fAskBeforeConnecting = info.info.askBeforeConnecting;
}
