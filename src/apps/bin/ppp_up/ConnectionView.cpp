/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "ConnectionView.h"
#include <MessageDriverSettingsUtils.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#include <PPPInterface.h>
#include <PPPManager.h>
#include <settings_tools.h>
#include <stl_algobase.h>
	// for max()


// GUI constants
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgCancel = 'CANC';
static const uint32 kMsgConnect = 'CONN';

// labels
static const char *kLabelSavePassword = "Save Password";
static const char *kLabelName = "Username: ";
static const char *kLabelPassword = "Password: ";
static const char *kLabelConnect = "Connect";
static const char *kLabelCancel = "Cancel";

// connection status strings
static const char *kTextConnecting = "Connecting...";
static const char *kTextConnectionEstablished = "Connection established.";
static const char *kTextNotConnected = "Not connected.";
static const char *kTextDeviceUpFailed = "Failed to connect.";
static const char *kTextAuthenticating = "Authenticating...";
static const char *kTextAuthenticationFailed = "Authentication failed!";
static const char *kTextConnectionLost = "Connection lost!";


static
status_t
up_thread(void *data)
{
	PPPInterface *interface = static_cast<PPPInterface*>(data);
	interface->Up();
	delete interface;
	return B_OK;
}


static
status_t
down_thread(void *data)
{
	PPPInterface *interface = static_cast<PPPInterface*>(data);
	interface->Down();
	delete interface;
	return B_OK;
}


ConnectionView::ConnectionView(BRect rect, const char *name, ppp_interface_id id,
		thread_id thread)
	: BView(rect, "ConnectionView", B_FOLLOW_NONE, 0),
	fInterfaceName(name),
	fID(id),
	fReportThread(thread),
	fConnecting(false),
	fKeepLabel(false),
	fReplyRequested(true)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	rect.bottom = rect.top
		+ 25 // space for topmost control
		+ 3 * 20 // size of controls
		+ 3 * 5; // space beween controls and bottom of box
	BBox *authenticationBox = new BBox(rect, "Authentication");
	rect = authenticationBox->Bounds();
	rect.InsetBy(10, 5);
	rect.top = 25;
	BView *authenticationView = new BView(rect, "authenticationView",
		B_FOLLOW_NONE, 0);
	authenticationView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect = authenticationView->Bounds();
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
	
	authenticationView->AddChild(fUsername);
	authenticationView->AddChild(fPassword);
	authenticationView->AddChild(fSavePassword);
	authenticationBox->AddChild(authenticationView);
	AddChild(authenticationBox);
	
	rect = authenticationBox->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 15;
	fAttemptView = new BStringView(rect, "AttemptView", AttemptString().String());
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
	Window()->SetDefaultButton(fConnectButton);
	
	rect.left = rect.right + 10;
	rect.right = rect.left + kDefaultButtonWidth;
	fCancelButton = new BButton(rect, "CancelButton", kLabelCancel,
		new BMessage(kMsgCancel));
	
	AddChild(fConnectButton);
	AddChild(fCancelButton);
	
	// initialize PTPSettings
	fSettings.LoadAddons(false);
	// add PPPUpAddon
	fAddon = new PPPUpAddon(&fSettings.Addons(), this);
	fSettings.Addons().AddPointer(DUN_TAB_ADDON_TYPE, fAddon);
	fSettings.Addons().AddPointer(DUN_DELETE_ON_QUIT, fAddon);
}


void
ConnectionView::AttachedToWindow()
{
	fConnectButton->SetTarget(this);
	fCancelButton->SetTarget(this);
	fSettings.LoadSettings(fInterfaceName.String(), false);
	
	if(fAddon->CountAuthenticators() == 0 || fAddon->HasPassword()
			|| fAddon->AskBeforeConnecting())
		Connect();
}


void
ConnectionView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_UPDATE: {
			UpdateStatus(message->FindInt32("code"));
			ppp_interface_id id;
			if(message->FindInt32("interface", reinterpret_cast<int32*>(&id)) == B_OK)
				fID = id;
		} break;
		
		case MSG_REPLY:
			message->SendReply(B_OK);
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
	fUsername->SetText(Addon()->Username());
	fPassword->SetText(Addon()->Password());
	fSavePassword->SetValue(Addon()->HasPassword());
	
	if(Addon()->CountAuthenticators() > 0) {
		fUsername->SetEnabled(true);
		fPassword->SetEnabled(true);
		fSavePassword->SetEnabled(true);
	} else {
		fUsername->SetEnabled(false);
		fPassword->SetEnabled(false);
		fSavePassword->SetEnabled(false);
	}
}


void
ConnectionView::Connect()
{
	fSettings.SaveSettingsToFile();
	
	// update interface profile
	BMessage settings, profile;
	fSettings.SaveSettings(&settings, &profile, true);
	driver_settings *temporaryProfile = MessageToDriverSettings(profile);
	PPPInterface *interface = new PPPInterface(PPPManager().CreateInterfaceWithName(
		fInterfaceName.String()));
	interface->SetProfile(temporaryProfile);
	free_driver_settings(temporaryProfile);
	
	fConnectButton->SetEnabled(false);
	fConnecting = true;
	
	if(fReplyRequested) {
		fReplyRequested = false;
		send_data(fReportThread, B_OK, NULL, 0);
		delete interface;
	} else {
		thread_id up = spawn_thread(up_thread, "up_thread", B_NORMAL_PRIORITY,
			interface);
		resume_thread(up);
	}
}


void
ConnectionView::Cancel()
{
	if(fReplyRequested) {
		fReplyRequested = false;
		send_data(fReportThread, B_ERROR, NULL, 0);
			// tell requestor to cancel connection attempt
	} else {
		PPPInterface *interface = new PPPInterface(fID);
		ppp_interface_info_t info;
		interface->GetInterfaceInfo(&info);
		
		if(info.info.phase < PPP_ESTABLISHED_PHASE) {
			thread_id down = spawn_thread(down_thread, "down_thread",
				B_NORMAL_PRIORITY, interface);
			resume_thread(down);
		} else
			delete interface;
	}
}


// Clean up before our window quits (called by ConnectionWindow).
void
ConnectionView::CleanUp()
{
	// TODO: finish clean-up; (DONE?)
	if(fReplyRequested)
		Cancel();
}


BString
ConnectionView::AttemptString() const
{
	PPPInterface interface(fID);
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	BString attempt;
	attempt << "Attempt " << info.info.connectRetry << " of " <<
		info.info.connectRetriesLimit;
	
	return attempt;
}


void
ConnectionView::UpdateStatus(int32 code)
{
	fAttemptView->SetText(AttemptString().String());
	
	switch(code) {
		case PPP_REPORT_UP_ABORTED:
		case PPP_REPORT_DEVICE_UP_FAILED:
		case PPP_REPORT_LOCAL_AUTHENTICATION_FAILED:
		case PPP_REPORT_PEER_AUTHENTICATION_FAILED:
		case PPP_REPORT_DOWN_SUCCESSFUL:
		case PPP_REPORT_CONNECTION_LOST:
			fConnectButton->SetEnabled(true);
		break;
		
		default:
			fConnectButton->SetEnabled(false);
	}
	
	// maybe the status string must not be changed (codes that set fKeepLabel to false
	// should still be handled)
	if(fKeepLabel && code != PPP_REPORT_GOING_UP && code != PPP_REPORT_UP_SUCCESSFUL)
		return;
	
	// only errors should set fKeepLabel to true
	switch(code) {
		case PPP_REPORT_GOING_UP:
			fKeepLabel = false;
			fStatusView->SetText(kTextConnecting);
		break;
		
		case PPP_REPORT_UP_SUCCESSFUL:
			fKeepLabel = false;
			fStatusView->SetText(kTextConnectionEstablished);
		break;
		
		case PPP_REPORT_UP_ABORTED:
		case PPP_REPORT_DOWN_SUCCESSFUL:
			fStatusView->SetText(kTextNotConnected);
		break;
		
		case PPP_REPORT_DEVICE_UP_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextDeviceUpFailed);
		break;
		
		case PPP_REPORT_LOCAL_AUTHENTICATION_REQUESTED:
		case PPP_REPORT_PEER_AUTHENTICATION_REQUESTED:
			fStatusView->SetText(kTextAuthenticating);
		break;
		
		case PPP_REPORT_LOCAL_AUTHENTICATION_FAILED:
		case PPP_REPORT_PEER_AUTHENTICATION_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextAuthenticationFailed);
		break;
		
		case PPP_REPORT_CONNECTION_LOST:
			fKeepLabel = true;
			fStatusView->SetText(kTextConnectionLost);
		break;
	}
}
