#include <StringView.h>
#include "InterfaceWin.h"
#include "NetListView.h"

InterfaceWin::InterfaceWin(const BRect &frame, const InterfaceData &data)
 :	BWindow(frame,"Interface Win",B_MODAL_WINDOW,
 			B_NOT_ANCHORED_ON_ACTIVATE | B_NOT_RESIZABLE,B_CURRENT_WORKSPACE),
 			fData(data.fName.String()) 			
{
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	BRect r(Bounds());
	
 	fData=data;
	BView *view=new BView(r,"Interface_View",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);
	
	float width,height;
	
	fName=new BStringView(BRect(5,5,6,6),"Interface_Name",data.fPrettyName.String());
	fName->GetPreferredSize(&width,&height);
	fName->ResizeTo(width,height);
	view->AddChild(fName);
	
	// There is a serious possibility that the name of the interface may be
	// longer than the current width of the window, especially at larger font
	// settings. If this is the case, we will end up resizing the window to fit it
	// all
	float maxwidth = MAX(width, r.right);
	
	float ypos = 5 + height + 20;
	fEnabled=new BCheckBox(BRect(5,ypos,6,ypos + 1),
							"Enabled","Interface enabled",
							new BMessage(kSOMETHING_HAS_CHANGED),
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fEnabled->GetPreferredSize(&width,&height);
	fEnabled->ResizeTo(width,height);
	view->AddChild(fEnabled);
	
	ypos += height + 10;
	fDHCP=new BRadioButton(BRect(5,ypos,6,ypos+1),"DHCP",
							"Obtain settings automatically",
							new BMessage(kSOMETHING_HAS_CHANGED));	
	fDHCP->GetPreferredSize(&width,&height);
	fDHCP->ResizeTo(width,height);
	view->AddChild(fDHCP);
	
	ypos += height + 1;
	fManual=new BRadioButton(BRect(5,ypos,6,ypos+1),"Manual","Specify settings",
							new BMessage(kSOMETHING_HAS_CHANGED));
	fManual->GetPreferredSize(&width,&height);
	fManual->ResizeTo(width,height);
	view->AddChild(fManual);
	
	ypos += height + 10;
	
	fIPAddress=new BTextControl(BRect(10,10,6,6),"IP address","IP address:",NULL,
								new BMessage(kSOMETHING_HAS_CHANGED),
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);  
	fIPAddress->GetPreferredSize(&width,&height);
	width = r.right - 15;
	
	fNetMask=new BTextControl(BRect(10,20 + height,width - 5,15 + (height*2)),
								"Subnet mask","Subnet mask:",NULL,
								new BMessage(kSOMETHING_HAS_CHANGED),
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);  
	
	fGateway=new BTextControl(BRect(10,30 + (height*2),width - 5,25 + (height*3)),
								"Gateway","Gateway:",NULL,
								new BMessage(kSOMETHING_HAS_CHANGED),
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);  
	
	fBox=new BBox(BRect(5,ypos,r.right - 5, ypos + fGateway->Frame().bottom + 10),
				"IP",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	view->AddChild(fBox);
	
	fBox->AddChild(fIPAddress);
	fBox->AddChild(fNetMask);
	fBox->AddChild(fGateway);
	
	// We have to place the ResizeTo() call down here because in R5, these calls
	// apparently don't do anything unless attached to a window. Grrr....
	fIPAddress->ResizeTo(width - 15, height);
	
	float dividersize = fIPAddress->StringWidth("Subnet mask:") + 10;
	fIPAddress->SetDivider(dividersize);
	fNetMask->SetDivider(dividersize);
	fGateway->SetDivider(dividersize);
	
	fIPAddress->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fNetMask->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fGateway->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	
	
	ypos = fBox->Frame().bottom + 10;
	
	// TODO: Finish the Configure code
	fConfig=new BButton(BRect(5,ypos,6,ypos+1),"Config","Configure", NULL);
	fConfig->GetPreferredSize(&width,&height);
	fConfig->ResizeTo(width,height);
	view->AddChild(fConfig);
	
	
	fDone=new BButton(BRect(0,0,1,1),"Done","Done",	new BMessage(kDone_inter_M),
						B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	
	fDone->ResizeToPreferred();
	fDone->MoveTo(r.right - 5 - fDone->Bounds().right,ypos);
	
	fCancel=new BButton(BRect(0,0,1,1),"Cancel","Cancel",
						new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
						
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fDone->Frame().left - 10 - fCancel->Bounds().right,ypos);
	view->AddChild(fCancel);
	view->AddChild(fDone);
	fDone->MakeDefault(true);
	
	ypos += height + 10;
	if(r.bottom < ypos || r.right < maxwidth)
		ResizeTo( MAX(r.right,maxwidth), MAX(ypos, r.bottom) );
}	

void InterfaceWin::MessageReceived(BMessage *message)
{
	switch(message->what) {
	
		case kDone_inter_M: {
			// TODO: Re-enable
/*			if (fChanged==true){
				fParentWindow->PostMessage(fParentWindow->kSOMETHING_HAS_CHANGED_M);
				
				NetListItem *item=dynamic_cast <NetListItem *> (fParentWindow->fInterfacesList->ItemAt(fParentWindow->fInterfacesList->CurrentSelection()));
				
				item->fIPADDRESS=fIPAddress->Text();
				item->fNETMASK 	=fNetMask->Text();
				
				if (fEnabled->Value()==0)
					item->fENABLED	=0;
				else
					item->fENABLED	=1;
				
				if (fDHCP->Value()==0)
					item->fDHCP=0;
				else
					item->fDHCP=1;			 
			}	
			Quit();
*/			break;
		}
		case kSOMETHING_HAS_CHANGED: {
		
			fChanged=true;
			break;
		}
	}			
}
