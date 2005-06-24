#include "passwordWindow.h"
#include <stdio.h>
#include "RadioButton.h"
#include "Alert.h"

void 
pwWindow::setup(void) 
{
	BView *owner=new BView(Bounds(),"ownerView",B_FOLLOW_NONE,B_WILL_DRAW);
	owner->SetViewColor(216,216,216);
	AddChild(owner);
	fUseNetwork=new BRadioButton(BRect(15,10,160,20),"useNetwork","Use Network password",new BMessage(kButton_changed),B_FOLLOW_NONE);
	fUseNetwork->SetValue(1);
	owner->AddChild(fUseNetwork);
	fUseCustom=new BRadioButton(BRect(30,50,130,60),"fUseCustom","Use custom password",new BMessage(kButton_changed),B_FOLLOW_NONE);

	fCustomBox=new BBox(BRect(10,30,270,105),"custBeBox",B_FOLLOW_NONE);
	fCustomBox->SetLabel(fUseCustom);
	fPassword=new BTextControl(BRect(10,20,250,35),"pwdCntrl","Password:",NULL,B_FOLLOW_NONE);
	fConfirm=new BTextControl(BRect(10,45,250,60),"fConfirmCntrl","Confirm fPassword:",NULL,B_FOLLOW_NONE);
	fPassword->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fPassword->SetDivider(90);
	fPassword->TextView()->HideTyping(true);
	fConfirm->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fConfirm->SetDivider(90);
	fConfirm->TextView()->HideTyping(true);
	fCustomBox->AddChild(fPassword);
	fCustomBox->AddChild(fConfirm);
	owner->AddChild(fCustomBox);

	fDone=new BButton(BRect(200,120,275,130),"done","Done",new BMessage (kDone_clicked),B_FOLLOW_NONE);
	fCancel=new BButton(BRect(115,120,190,130),"cancel","Cancel",new BMessage (kCancel_clicked),B_FOLLOW_NONE);
	owner->AddChild(fDone);
	owner->AddChild(fCancel);
	fDone->MakeDefault(true);
	update();
}

void 
pwWindow::update(void) 
{
	fUseNetPassword=(fUseCustom->Value()>0);
	fConfirm->SetEnabled(fUseNetPassword);
	fPassword->SetEnabled(fUseNetPassword);
}


void 
pwWindow::MessageReceived(BMessage *message) 
{
  switch(message->what) {
    case kDone_clicked:
		if (fUseCustom->Value())
			if (strcmp(fPassword->Text(),fConfirm->Text())) {
				BAlert *alert=new BAlert("noMatch","Passwords don't match. Try again.","OK");
				alert->Go();
			} else {
				fThePassword=fPassword->Text();
				Hide();
			}
		else {
			fPassword->SetText("");
			fConfirm->SetText("");
			Hide();
			}
		break;
    case kCancel_clicked:
		fPassword->SetText("");
		fConfirm->SetText("");
		Hide();
		break;
	case kButton_changed:
		update();
		break;
	case kShow:
		Show();
		break;
	case kPopulate:
		message->ReplaceString("lockpassword", ((fUseNetPassword)?"":fThePassword)); 
		message->ReplaceString("lockmethod", (fUseNetPassword?"network":"custom")); 
		message->SendReply(message);
		break;
	case kUtilize: {
		BString temp;
		message->FindString("lockmethod",&temp);
		fUseNetPassword=(temp=="custom");
		if (!fUseNetPassword) {
			message->FindString("lockpassword",&temp);
			fThePassword=temp;
		}
		break;
	}
    default:
      	BWindow::MessageReceived(message);
      	break;
  }
}
