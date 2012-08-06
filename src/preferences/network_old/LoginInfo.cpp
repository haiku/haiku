#include "LoginInfo.h"
#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <Alert.h>
#include "NetworkWindow.h"

LoginInfo::LoginInfo(void)
 :BWindow(BRect(410,200,710,335),"Login Info",B_MODAL_WINDOW,B_NOT_ANCHORED_ON_ACTIVATE
 		 | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE)
{
	fView=new BView(Bounds(),"Login_Info_View",B_FOLLOW_ALL_SIDES,B_WILL_DRAW); 
	fName=new BTextControl(BRect(10,10,290,30),"User_Name",
									"User Name",NULL,new BMessage());  
	fPassword=new BTextControl(BRect(10,40,290,60),"Password",
										"Password",NULL,new BMessage());
	fPassword->TextView()->HideTyping(true);
	fConfirm=new BTextControl(BRect(10,70,290,90),"Confirm Password",
										"Confirm Password",NULL,new BMessage());  
	fConfirm->TextView()->HideTyping(true);
	fCancel=new BButton(BRect(100,100,190,120),"Cancel","Cancel",
									new BMessage(kLogin_Info_Cancel_M));
	fDone=new BButton(BRect(200,100,290,120),"Done","Done",
									new BMessage(kLogin_Info_Done_M));
	
	fDone->MakeDefault(true);
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fView->AddChild(fName);
	fView->AddChild(fPassword);
	fView->AddChild(fConfirm);
	fView->AddChild(fCancel);
	fView->AddChild(fDone);		
	
	AddChild(fView);
}

void LoginInfo::MessageReceived(BMessage *message)
{
	switch (message->what) {
	
		case kLogin_Info_Cancel_M: {
			Quit();
			break;
		}
		
		case kLogin_Info_Done_M: {		
			
			const char *login_info_name_S 	  = fName->Text();
			const char *login_info_password_S = fPassword  ->Text();
			const char *login_info_confirm_S  = fConfirm   ->Text();
			
			if (strlen(login_info_name_S)>0 && 
				strlen(login_info_password_S)>0 &&
				strlen(login_info_confirm_S)>0 ) {
				
					if (strcmp(login_info_password_S,login_info_confirm_S) == 0) {
					
						fParentWindow->flogin_info_S[0] = login_info_name_S;
						fParentWindow->flogin_info_S[1] = login_info_password_S;											
						fParentWindow->PostMessage(fParentWindow->kSOMETHING_HAS_CHANGED_M);
						Quit();
					}
					else {
						BAlert *alert = new BAlert("Login Info Alert",
							"Passwords don't match. Please try again","OK");
						alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
						alert->Go();
					}	
					
			}
			else {
			
				BAlert *alert = new BAlert("Login Info Alert",
					"You didn't fill in all the fields","Oops...", NULL, NULL,
					B_WIDTH_FROM_WIDEST,B_INFO_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}	
			break;
		}
	}			
}
