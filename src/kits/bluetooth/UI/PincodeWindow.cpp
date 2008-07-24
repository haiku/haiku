
/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

#include <String.h>
#include <Message.h>
#include <Application.h>

#include <Button.h>
#include <InterfaceDefs.h>
#include <StringView.h>
#include <TextControl.h>

#include <bluetooth/RemoteDevice.h>
#include <bluetooth/LocalDevice.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/bluetooth_error.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <PincodeWindow.h>
#include <bluetoothserver_p.h>
#include <CommandManager.h>

#define MSG_ACCEPT_BUTTON 'acCp'
#define MSG_CANCEL_BUTTON 'mVch'

#define H_SEPARATION  15
#define V_SEPARATION  10
#define BD_ADDR_LABEL "BD_ADDR: "

namespace Bluetooth {

PincodeView::PincodeView(BRect rect) : BView(rect,"View", B_FOLLOW_NONE, B_WILL_DRAW ), fMessage(NULL)
{
	BRect rect1;
	BRect rect2;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fMessage = new BStringView(BRect(0,0,5,5),"Pincode","Please enter the pincode ...", B_FOLLOW_ALL_SIDES);
	fMessage->SetFont(be_bold_font);
	fMessage->ResizeToPreferred();
	fMessage->MoveBy(20, 20);
	rect1 = fMessage->Frame();
	
	fRemoteInfo = new BStringView(BRect(rect1.left, rect1.bottom + V_SEPARATION , 0 , 0), 
                                      "bdaddr","BD_ADDR: ", B_FOLLOW_ALL_SIDES);
	fRemoteInfo->ResizeToPreferred();
	rect1 = fRemoteInfo->Frame();
	
	// TODO: IT CANNOT BE MORE THAN 16 BYTES 
	fPincodeText = new BTextControl(BRect(rect1.left, rect1.bottom + V_SEPARATION , rect1.right, rect1.bottom + V_SEPARATION + 20), 
                                        "pincode TextControl","PIN code:", "5555", NULL);
	fPincodeText->ResizeToPreferred();
	rect1 = fPincodeText->Frame();

	fAcceptButton = new BButton(BRect(rect1.left , rect1.bottom + V_SEPARATION ,0, 0 ),
                                    "fAcceptButton","Pair",new BMessage(MSG_ACCEPT_BUTTON));
	fAcceptButton->ResizeToPreferred();
	rect1 = fAcceptButton->Frame();
	
	fCancelButton = new BButton(BRect(rect1.right + H_SEPARATION , rect1.top , 0 , 0 ),
                                    "fCancelButton","Cancel",new BMessage(MSG_CANCEL_BUTTON));
	fCancelButton->ResizeToPreferred();

	this->AddChild(fMessage);
	this->AddChild(fPincodeText);
	this->AddChild(fAcceptButton);
	this->AddChild(fCancelButton);

	this->AddChild(fRemoteInfo);

	// Now resize the the view according all what we found here
	rect1 = fMessage->Frame();
	rect2 = fCancelButton->Frame();
	ResizeTo(rect1.right + 15 , rect2.bottom +15 );

}

void PincodeView::SetBDaddr(const char* address){

				BString label;
				
				label << BD_ADDR_LABEL << address;
				printf("++ %s\n",label.String());
				fRemoteInfo->SetText(label.String());
				fRemoteInfo->ResizeToPreferred();
				Invalidate();
						
}

#if 0
#pragma mark -
#endif

PincodeWindow::PincodeWindow(bdaddr_t address, hci_id hid) :
		BWindow(BRect(700, 100, 900, 150), "Pincode Request", 
		                                   B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        		                           B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_NOT_RESIZABLE,
                                           B_ALL_WORKSPACES), fBdaddr(address), fHid(hid)
{
	fView = new PincodeView(Bounds());
	AddChild(fView);
	// readapt ourselves to what the view needs
	ResizeTo( fView->Bounds().right , fView->Bounds().bottom );
	
	// TODO: Get more info about device" ote name/features/encry/auth... etc
	fView->SetBDaddr( bdaddrUtils::ToString(fBdaddr) );
};

void PincodeWindow::MessageReceived(BMessage *msg)
{
//	status_t err = B_OK;
	
	switch(msg->what)
	{
		case MSG_ACCEPT_BUTTON:
		{
			BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
			BMessage reply;
			size_t   size;
			int8     bt_status = BT_ERROR;
				
			void* command = buildPinCodeRequestReply(fBdaddr, strlen(fView->fPincodeText->Text()), 
													(char*)fView->fPincodeText->Text(), &size);
		
			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", fHid);
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY));
		
			if (be_app_messenger.SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					PostMessage(B_QUIT_REQUESTED);
				}
				// TODO: something failed here
			}


		}
		break;


		case MSG_CANCEL_BUTTON:
		{
			BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
			BMessage reply;
			size_t   size;
			int8     bt_status = BT_ERROR;
				
			void* command = buildPinCodeRequestNegativeReply(fBdaddr, &size);
		
			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", fHid);
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY));
		
			if (be_app_messenger.SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					PostMessage(B_QUIT_REQUESTED);
				}
				// TODO: something failed here
			}

		}
		break;

		default:
			BWindow::MessageReceived(msg);
		break;
	}
}



bool PincodeWindow::QuitRequested()
{

	return BWindow::QuitRequested();
};

}

