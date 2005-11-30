/*

LocationView - View for the Location Controls on DUNWindow

Authors: Misza (misza@ihug.com.au) 
		 Sikosis (beos@gravity24hr.com)		 

(C) 2002 OpenBeOS under MIT license

*/


#include "LocationView.h"

using std::cout;
using std::endl;


// LocationView -- View inside DUNWindow
LocationView::LocationView() : BView(BRect(55,31,285,87),"locationview",B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	  
	BRect line1(0,5,125,20);
	disablecallwaiting = new BCheckBox(line1,"disablecallwaiting","Disable call waiting:",new BMessage(DCW_CHKBOX));
	
	AddChild(disablecallwaiting);
	
	line1.OffsetBy(30,0);
	
	dcwvalue = new BTextControl(BRect(125,5,175,20),"dcwvalue",NULL,NULL,new BMessage(CHANGE_DCW));
	AddChild(dcwvalue);
	
	line1.OffsetBy(30,0);
	
	dcwlist = new BPopUpMenu("");
	
	dcwone = new BMenuItem("*70,",new BMessage(DCW_ITEMS));
    dcwtwo = new BMenuItem("70#,",new BMessage(DCW_ITEMS));
    dcwthree = new BMenuItem("1170,",new BMessage(DCW_ITEMS));
	dcwlist->AddItem(dcwone);
	dcwlist->AddItem(dcwtwo);
	dcwlist->AddItem(dcwthree);
	
	dcwlistfield = new BMenuField(BRect(180,5,240,20),
	NULL,NULL,dcwlist);
	dcwone->SetMarked(true);
	AddChild(dcwlistfield);
	
	//LINE 2
	BRect line2(0,30,125,45);
	
	dialoutprefix = new BCheckBox(line2,"dialoutprefix",
	"Dial out prefix:",new BMessage(DOP_CHKBOX));
	AddChild(dialoutprefix);
	
	dopvalue = new BTextControl(BRect(125,30,175,45),"dopvalue",NULL,NULL,
	new BMessage(CHANGE_DOP));
	AddChild(dopvalue);
	
	doplist = new BPopUpMenu("");
	
	dopone = new BMenuItem("1,",new BMessage(DOP_ITEMS));
    doptwo = new BMenuItem("9,",new BMessage(DOP_ITEMS));
    dopthree = new BMenuItem("9,1,",new BMessage(DOP_ITEMS));
	
	doplist->AddItem(dopone);
	doplist->AddItem(doptwo);
	doplist->AddItem(dopthree);
	doplistfield = new BMenuField(BRect(180,30,240,45),
	NULL,NULL,doplist);
	dopone->SetMarked(true);
	AddChild(doplistfield);

	//turned off by default
   	dcwlistfield->SetEnabled(false);
	dcwvalue->SetEnabled(false);
   	
   	doplistfield->SetEnabled(false);
	dopvalue->SetEnabled(false);
}
// ------------------------------------------------------------------------------- //


// LocationView::MessageReceived -- Receives Messages
void LocationView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case DCW_CHKBOX:
			if(disablecallwaiting->Value() == B_CONTROL_ON)
			{
				cout << "dcw on" << endl;
				dcwlistfield->SetEnabled(true);
				dcwvalue->SetEnabled(true);
			} else {
				cout << "dcw off" << endl;
				dcwlistfield->SetEnabled(false);
				dcwvalue->SetEnabled(false);
			}
			break;
		case DOP_CHKBOX:
			if(dialoutprefix->Value() == B_CONTROL_ON)
			{
				cout << "dop on" << endl;
				doplistfield->SetEnabled(true);
				dopvalue->SetEnabled(true);
			} else {
				cout << "dop off" << endl;
				doplistfield->SetEnabled(false);
				dopvalue->SetEnabled(false);
			}
			break;
		case CHANGE_DOP:
			cout << "DOP is set to: " << dopvalue->Text() << endl;
			break;
		case CHANGE_DCW:
			cout << "DCW is set to: " << dcwvalue->Text() << endl;
			break;
		case DCW_ITEMS:
			if(dcwone->IsMarked())
				dcwvalue->SetText("*70,");
			else if(dcwtwo->IsMarked())
				dcwvalue->SetText("70#,");
			else if(dcwthree->IsMarked())
				dcwvalue->SetText("1170,");
			dcwvalue->MakeFocus();	
			cout << "DCW is set to: " << dcwvalue->Text() << endl;			
			break;
		case DOP_ITEMS:
			if(dopone->IsMarked())
				dopvalue->SetText("1,");
			else if(doptwo->IsMarked())
				dopvalue->SetText("9,");
			else if(dopthree->IsMarked())
				dopvalue->SetText("9,1,");
			dopvalue->MakeFocus();	
			cout << "DOP is set to: " << dopvalue->Text() << endl;
			break;
		default:
    	    BView::MessageReceived(msg);
        	break;
	}
}
// ------------------------------------------------------------------------------- //

// LocationView::AttachedToWindow -- facilitates the LocationView to be able to receive messages in MessageReceived() 
void LocationView::AttachedToWindow()
{
	disablecallwaiting->SetTarget(this);
	dcwvalue->SetTarget(this);
	dcwone->SetTarget(this);
	dcwtwo->SetTarget(this);
	dcwthree->SetTarget(this);
	
	dialoutprefix->SetTarget(this);
	dopvalue->SetTarget(this);
	dopone->SetTarget(this);
	doptwo->SetTarget(this);
	dopthree->SetTarget(this);
}
// ------------------------------------------------------------------------------- //
