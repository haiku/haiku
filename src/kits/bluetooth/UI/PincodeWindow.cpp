
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
	BRect rect;
	BRect rect2;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

  	fMessage = new BStringView(BRect(0,0,5,5),"Pincode","Please enter the pincode ...", B_FOLLOW_ALL_SIDES);
	fMessage->SetFont(be_bold_font);
	fMessage->ResizeToPreferred();
	fMessage->MoveBy(20, 20);
	rect = fMessage->Frame();
	
  	fRemoteInfo = new BStringView(BRect(rect.left, rect.bottom + V_SEPARATION , 0 , 0), 
                                      "bdaddr","BD_ADDR: ", B_FOLLOW_ALL_SIDES);
	fRemoteInfo->ResizeToPreferred();
	rect = fRemoteInfo->Frame();
	
	// TODO: IT CANNOT BE MORE THAN 16 BYTES 
	fPincodeText = new BTextControl(BRect(rect.left, rect.bottom + V_SEPARATION , 0, 0), 
                                        "pincode TextControl","PIN code:", "", NULL);
	fPincodeText->ResizeToPreferred();
	rect = fPincodeText->Frame();
		
	fAcceptButton = new BButton(BRect(rect.left , rect.bottom + V_SEPARATION ,0, 0 ),
                                    "fAcceptButton","Pair",new BMessage(MSG_ACCEPT_BUTTON));
	fAcceptButton->ResizeToPreferred();
	rect = fAcceptButton->Frame();
	
	fCancelButton = new BButton(BRect(rect.right + H_SEPARATION , rect.top , 0 , 0 ),
                                    "fCancelButton","Cancel",new BMessage(MSG_CANCEL_BUTTON));
	fCancelButton->ResizeToPreferred();
	rect = fCancelButton->Frame();

  	this->AddChild(fMessage);
	this->AddChild(fPincodeText);
  	this->AddChild(fAcceptButton);
  	this->AddChild(fCancelButton);

  	this->AddChild(fRemoteInfo);

  	// Now resize the the view according all what we found here
	rect  = fMessage->Frame();
	rect2 = fCancelButton->Frame();
  	ResizeTo(rect.right + 15 , rect2.bottom +15 );

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

PincodeWindow::PincodeWindow(RemoteDevice* rDevice) :
		BWindow(BRect(700, 100, 900, 150), "Pincode Request",  B_MODAL_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
                                           B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE ,
                                           B_ALL_WORKSPACES), fRemoteDevice(rDevice)
{
	fView = new PincodeView(Bounds());
	AddChild(fView);
	// readapt ourselves to what the view needs
	ResizeTo( fView->Bounds().right , fView->Bounds().bottom );
	
	// TODO: Get more info about device" ote name/features/encry/auth... etc
	fView->SetBDaddr( bdaddrUtils::ToString(rDevice->GetBluetoothAddress()) );
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
				
			void* command = buildPinCodeRequestReply(fRemoteDevice->GetBluetoothAddress(), strlen(fView->fPincodeText->Text()), 
													(char[16])fView->fPincodeText->Text(), &size);
		
			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", (fRemoteDevice->GetLocalDeviceOwner())->GetID());
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY));
		
			if (fMessenger->SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					break;
				}
				// TODO: something failed here
			}

			QuitRequested();
		}
		break;


		case MSG_CANCEL_BUTTON:
		{
			BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
			BMessage reply;
			size_t   size;
			int8     bt_status = BT_ERROR;
				
			void* command = buildPinCodeRequestNegativeReply(fRemoteDevice->GetBluetoothAddress(), &size);
		
			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", (fRemoteDevice->GetLocalDeviceOwner())->GetID());
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY));
		
			if (fMessenger->SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					break;
				}
				// TODO: something failed here
			}

			QuitRequested();
		}
		break;

		default:
			BWindow::MessageReceived(msg);
		break;
	}
}



bool PincodeWindow::QuitRequested()
{
	Lock();
	Quit();
	return (true);
};

}

