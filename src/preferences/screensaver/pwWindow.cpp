#include "pwWindow.h"
#include <stdio.h>
#include "RadioButton.h"
#include "Alert.h"

void pwWindow::setup(void)
{
	BView *owner=new BView(Bounds(),"ownerView",B_FOLLOW_NONE,B_WILL_DRAW);
	owner->SetViewColor(216,216,216);
	AddChild(owner);
	useNetwork=new BRadioButton(BRect(15,10,160,20),"useNetwork","Use Network password",new BMessage(BUTTON_CHANGED),B_FOLLOW_NONE);
	useNetwork->SetValue(1);
	owner->AddChild(useNetwork);
	useCustom=new BRadioButton(BRect(30,50,130,60),"useCustom","Use custom password",new BMessage(BUTTON_CHANGED),B_FOLLOW_NONE);

	customBox=new BBox(BRect(10,30,270,105),"custBeBox",B_FOLLOW_NONE);
	customBox->SetLabel(useCustom);
	password=new BTextControl(BRect(10,20,250,35),"pwdCntrl","Password:",NULL,B_FOLLOW_NONE);
	confirm=new BTextControl(BRect(10,45,250,60),"confirmCntrl","Confirm password:",NULL,B_FOLLOW_NONE);
	password->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	password->SetDivider(90);
	password->TextView()->HideTyping(true);
	confirm->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	confirm->SetDivider(90);
	confirm->TextView()->HideTyping(true);
	customBox->AddChild(password);
	customBox->AddChild(confirm);
	owner->AddChild(customBox);

	done=new BButton(BRect(200,120,275,130),"done","Done",new BMessage (DONE_CLICKED),B_FOLLOW_NONE);
	cancel=new BButton(BRect(115,120,190,130),"cancel","Cancel",new BMessage (CANCEL_CLICKED),B_FOLLOW_NONE);
	owner->AddChild(done);
	owner->AddChild(cancel);
	done->MakeDefault(true);
	update();
}

void pwWindow::update(void)
{
	useNetPassword=(useCustom->Value()>0);
	confirm->SetEnabled(useNetPassword);
	password->SetEnabled(useNetPassword);
}

void pwWindow::MessageReceived(BMessage *message)
{
  switch(message->what)
  {
    case DONE_CLICKED:
		if (useCustom->Value())
			if (strcmp(password->Text(),confirm->Text()))
				{
				BAlert *alert=new BAlert("noMatch","Passwords don't match. Try again.","OK");
				alert->Go();
				}
			else
				{
				thePassword=password->Text();
				Hide();
				}
		else
			{
			password->SetText("");
			confirm->SetText("");
			Hide();
			}
		break;
    case CANCEL_CLICKED:
		password->SetText("");
		confirm->SetText("");
		Hide();
		break;
	case BUTTON_CHANGED:
		update();
		break;
	case SHOW:
		Show();
		break;
	case POPULATE:
		message->ReplaceString("lockpassword", ((useNetPassword)?"":thePassword)); 
		message->ReplaceString("lockmethod", (useNetPassword?"network":"custom")); 
		message->SendReply(message);
		break;
	case UTILIZE:
		{
		BString temp;
		message->FindString("lockmethod",&temp);
		useNetPassword=(temp=="custom");
		if (!useNetPassword)
			{
			message->FindString("lockpassword",&temp);
			thePassword=temp;
			}
		break;
		}
    default:
      	BWindow::MessageReceived(message);
      	break;
  }
}
