/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

#include "ConnectionView.h"
#include <MessageDriverSettingsUtils.h>

#include <Application.h>
#include <Button.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#include <PPPInterface.h>
#include <settings_tools.h>


static const char *kLabelConnect = "Connect";
static const char *kLabelCancel = "Cancel";

static const uint32 kMsgCancel = 'CANC';
static const uint32 kMsgConnect = 'CONN';

static const uint32 kDefaultButtonWidth = 80;


ConnectionView::ConnectionView(BRect rect, ppp_interface_id id, thread_id thread,
		DialUpView *dun)
	: BView(rect, "ConnectionView", B_FOLLOW_NONE, 0),
	fID(id),
	fRequestThread(thread),
	fDUNView(dun),
	fConnecting(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	fAuthenticationView = dun->AuthenticationView();
	fAuthenticationView->RemoveSelf();
	fAuthenticationView->MoveTo(5, 5);
	AddChild(fAuthenticationView);
	
	rect.top = fAuthenticationView->Frame().bottom + 15;
	rect.bottom = rect.top + 15;
	fAttemptView = new BStringView(rect, "AttemptView", AttemptString().String());
	AddChild(fAttemptView);
	
	rect.top = rect.bottom + 5;
	fStatusView = dun->StatusView();
	fStatusView->RemoveSelf();
	fStatusView->MoveTo(5, rect.top);
	AddChild(fStatusView);
	
	rect.top = fStatusView->Frame().bottom + 10;
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
}


void
ConnectionView::AttachedToWindow()
{
	fConnectButton->SetTarget(this);
	fCancelButton->SetTarget(this);
	
	if(fDUNView->NeedsRequest() == false)
		Connect();
}


void
ConnectionView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_UPDATE:
			Update();
		break;
		
		case MSG_UP_FAILED:
			UpFailed();
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


void
ConnectionView::Update()
{
	fAttemptView->SetText(AttemptString().String());
}


void
ConnectionView::UpFailed()
{
	Update();
	fConnectButton->SetEnabled(true);
}


void
ConnectionView::Connect()
{
	BMessage settings, profile;
	fDUNView->SaveSettings(&settings, &profile, true);
	driver_settings *temporaryProfile = MessageToDriverSettings(profile);
	PPPInterface interface(fID);
	interface.SetProfile(temporaryProfile);
	free_driver_settings(temporaryProfile);
	
	fConnectButton->SetEnabled(false);
	fConnecting = true;
	
	send_data(fRequestThread, B_OK, NULL, 0);
}


void
ConnectionView::Cancel()
{
	if(fConnecting) {
		// only disconnect if we are not connected yet
		PPPInterface interface(fID);
		ppp_interface_info_t info;
		interface.GetInterfaceInfo(&info);
		
		if(info.info.phase < PPP_ESTABLISHED_PHASE) {
			interface.Down();
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
	} else {
		send_data(fRequestThread, B_ERROR, NULL, 0);
			// tell requestor to cancel connection attempt
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}


// Clean up before our window quits (called by ConnectionWindow).
void
ConnectionView::CleanUp()
{
	// give the "stolen" views back to make sure they are not deleted too early
	fAuthenticationView->RemoveSelf();
	fDUNView->AddChild(fAuthenticationView);
	fStatusView->RemoveSelf();
	fDUNView->AddChild(fStatusView);
}


BString
ConnectionView::AttemptString() const
{
	PPPInterface interface(fID);
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	BString attempt;
	attempt << "Attempt " << info.info.dialRetry << " of " <<
		info.info.dialRetriesLimit;
	
	return attempt;
}
