#include "AutoConfigWindow.h"

#include "AutoConfig.h"
#include "AutoConfigView.h"

#include <Alert.h>
#include <Application.h>
#include <MailSettings.h>
#include <Message.h>

#include <File.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>


AutoConfigWindow::AutoConfigWindow(BRect rect, BWindow *parent)
	:	BWindow(rect, "Create new account", B_TITLED_WINDOW_LOOK,
				B_MODAL_APP_WINDOW_FEEL,
			 	B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AVOID_FRONT,
				B_ALL_WORKSPACES),
		fParentWindow(parent),
		fAccount(NULL),
		fMainConfigState(true),
		fServerConfigState(false),
		fAutoConfigServer(true)
{
	fRootView = new BView(Bounds(), "root auto config view",
								B_FOLLOW_ALL_SIDES, B_NAVIGABLE);
	fRootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fRootView);
	
	int32 buttonHeight = 25;
	int32 buttonWidth = 50;
	BRect buttonRect = Bounds();
	buttonRect.InsetBy(5,5);
	buttonRect.top = buttonRect.bottom - buttonHeight;
	buttonRect.left = buttonRect.right - 2 * buttonWidth - 5;
	buttonRect.right = buttonRect.left + buttonWidth;
	fBackButton = new BButton(buttonRect, "back", "Back",
								new BMessage(kBackMsg));
	fBackButton->SetEnabled(false);
	fRootView->AddChild(fBackButton);
	
	buttonRect.left+= 5 + buttonWidth;
	buttonRect.right = buttonRect.left + buttonWidth;
	fNextButton = new BButton(buttonRect, "ok", "OK", new BMessage(kOkMsg));
	fNextButton->MakeDefault(true);
	fRootView->AddChild(fNextButton);
	
	fBoxRect = Bounds();
	fBoxRect.InsetBy(5,5);
	fBoxRect.bottom-= buttonHeight + 5;
	
	fMainView = new AutoConfigView(fBoxRect, fAutoConfig);
	fMainView->SetLabel("Account settings");
	fRootView->AddChild(fMainView);
	
	// Add a shortcut to close the window using Command-W
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
}


AutoConfigWindow::~AutoConfigWindow()
{

}


void
AutoConfigWindow::MessageReceived(BMessage* msg)
{
	status_t status = B_ERROR;
	BAlert* invalidMailAlert = NULL;
	switch (msg->what)
	{
		case kOkMsg:
		{
			if (fMainConfigState) {
				fMainView->GetBasicAccountInfo(fAccountInfo);
				if (!fMainView->IsValidMailAddress(fAccountInfo.email)) {
					invalidMailAlert = new BAlert("invalidMailAlert",
													"Enter a valid e-mail address.",
													"OK");
					invalidMailAlert->Go();
					return;
				}
				if (fAutoConfigServer) {
					status = fAutoConfig.GetInfoFromMailAddress(fAccountInfo.email.String(),
																&(fAccountInfo.providerInfo));
				}
				if(status == B_OK){
					fParentWindow->Lock();
					GenerateBasicAccount();
					fParentWindow->Unlock();
					Quit();
				}
				fMainConfigState = false;
				fServerConfigState = true;
				fMainView->Hide();
				
				fServerView = new ServerSettingsView(fBoxRect, fAccountInfo);
				fRootView->AddChild(fServerView);
				
				fBackButton->SetEnabled(true);
			}
			else{
				fServerView->GetServerInfo(fAccountInfo);
				fParentWindow->Lock();
				GenerateBasicAccount();
				fParentWindow->Unlock();
				Quit();
			}
			break;
		}
		case kBackMsg:
		{
			if(fServerConfigState){
				fServerView->GetServerInfo(fAccountInfo);

				fMainConfigState = true;
				fServerConfigState = false;
								
				fRootView->RemoveChild(fServerView);
				delete fServerView;
				
				fMainView->Show();
				fBackButton->SetEnabled(false);
			}
			break;
		}
		case kServerChangedMsg:
		{
			fAutoConfigServer = false;
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool 
AutoConfigWindow::QuitRequested(void)
{
	return true;
}




Account*
AutoConfigWindow::GenerateBasicAccount()
{
	if(!fAccount) {
		fAccount = Accounts::NewAccount();
		fAccount->SetType(fAccountInfo.type);
		fAccount->SetName(fAccountInfo.accountName.String());
		fAccount->SetRealName(fAccountInfo.name.String());
		fAccount->SetReturnAddress(fAccountInfo.email.String());
				
		BString inServerName;
		int32 authType = 0;
		int32 ssl = 0;
		if (fAccountInfo.inboundType == IMAP) {
			inServerName = fAccountInfo.providerInfo.imap_server;
			ssl = fAccountInfo.providerInfo.ssl_imap;
		} else {
			inServerName = fAccountInfo.providerInfo.pop_server;
			authType = fAccountInfo.providerInfo.authentification_pop;
			ssl = fAccountInfo.providerInfo.ssl_pop;
		}
		if (fAccountInfo.type == INBOUND_TYPE
				|| fAccountInfo.type == IN_AND_OUTBOUND_TYPE) {
			BMailChain *inbound = fAccount->Inbound();
			if (inbound != NULL) {
				BMessage inboundArchive;
				inboundArchive.AddString("server", inServerName);
				inboundArchive.AddInt32("auth_method", authType);
				inboundArchive.AddInt32("flavor", ssl);
				inboundArchive.AddString("username", fAccountInfo.loginName);
				inboundArchive.AddString("password", fAccountInfo.password);
				inboundArchive.AddBool("leave_mail_on_server", true);
				inboundArchive.AddBool("delete_remote_when_local", true);
				inbound->SetFilter(0, inboundArchive, fAccountInfo.inboundProtocol);
			}
		}

		if (fAccountInfo.type == OUTBOUND_TYPE
				|| fAccountInfo.type == IN_AND_OUTBOUND_TYPE) {
			BMailChain *outbound = fAccount->Outbound();
			if (outbound != NULL) {
				BMessage outboundArchive;
				outboundArchive.AddString("server",
											fAccountInfo.providerInfo.smtp_server);
				outboundArchive.AddString("username", fAccountInfo.loginName);
				outboundArchive.AddString("password", fAccountInfo.password);
				outboundArchive.AddInt32("auth_method",
											fAccountInfo.providerInfo.authentification_smtp);
				outboundArchive.AddInt32("flavor", fAccountInfo.providerInfo.ssl_smtp);
				outbound->SetFilter(1, outboundArchive,
										fAccountInfo.outboundProtocol);
			}
		}
	}
	return fAccount;
}
