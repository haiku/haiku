/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

#include <String.h>
#include <Message.h>
#include <Application.h>

#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceDefs.h>
#include <SpaceLayoutItem.h>
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


#define H_SEPARATION  15
#define V_SEPARATION  10
#define BD_ADDR_LABEL "BD_ADDR: "


static const uint32 skMessageAcceptButton = 'acCp';
static const uint32 skMessageCancelButton = 'mVch';


namespace Bluetooth {


#if 0
#pragma mark -
#endif


PincodeWindow::PincodeWindow(bdaddr_t address, hci_id hid)
	: BWindow(BRect(800, 200, 900, 300), "Pincode request",
		B_FLOATING_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE|  B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES), fBdaddr(address), fHid(hid)
{
	InitUI();

	// TODO: Get more info about device" ote name/features/encry/auth... etc
	SetBDaddr(bdaddrUtils::ToString(fBdaddr));

}


PincodeWindow::PincodeWindow(RemoteDevice* rDevice)
	: BWindow(BRect(800, 200, 900, 300), "Pincode request",
		B_FLOATING_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
		| B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES)
{
	// TODO: Get more info about device" ote name/features/encry/auth... etc
	SetBDaddr(bdaddrUtils::ToString(rDevice->GetBluetoothAddress()));
	fHid = (rDevice->GetLocalDeviceOwner())->ID();
}


void
PincodeWindow::InitUI()
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fMessage = new BStringView("Pincode", "Please enter the pincode ...");
	fMessage->SetFont(be_bold_font);

	fRemoteInfo = new BStringView("bdaddr","BD_ADDR: ");

	// TODO: Pincode cannot be more than 16 bytes
	fPincodeText = new BTextControl("pincode TextControl", "PIN code:",
		"5555", NULL);

	fAcceptButton = new BButton("fAcceptButton", "Pair",
		new BMessage(skMessageAcceptButton));

	fCancelButton = new BButton("fCancelButton", "Cancel",
		new BMessage(skMessageCancelButton));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
				.Add(fMessage)
				.Add(fRemoteInfo)
				.Add(fPincodeText)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.AddGlue()
					.Add(fCancelButton)
					.Add(fAcceptButton)
					.SetInsets(5, 5, 5, 5)
				)
			.SetInsets(15, 15, 15, 15)
	);
}


void
PincodeWindow::MessageReceived(BMessage *msg)
{
//	status_t err = B_OK;

	switch(msg->what)
	{
		case skMessageAcceptButton:
		{
			BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
			BMessage reply;
			size_t size;
			int8 bt_status = BT_ERROR;

			void* command = buildPinCodeRequestReply(fBdaddr,
				strlen(fPincodeText->Text()),
				(char*)fPincodeText->Text(), &size);

			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", fHid);
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
				OCF_PIN_CODE_REPLY));

			// we reside in the server
			if (be_app_messenger.SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					PostMessage(B_QUIT_REQUESTED);
				}
				// TODO: something failed here
			}
			break;
		}

		case skMessageCancelButton:
		{
			BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
			BMessage reply;
			size_t size;
			int8 bt_status = BT_ERROR;

			void* command = buildPinCodeRequestNegativeReply(fBdaddr, &size);

			if (command == NULL) {
				break;
			}

			request.AddInt32("hci_id", fHid);
			request.AddData("raw command", B_ANY_TYPE, command, size);
			request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
			request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
				OCF_PIN_CODE_NEG_REPLY));

			if (be_app_messenger.SendMessage(&request, &reply) == B_OK) {
				if (reply.FindInt8("status", &bt_status ) == B_OK ) {
					PostMessage(B_QUIT_REQUESTED);
				}
				// TODO: something failed here
			}
			break;
		}

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool PincodeWindow::QuitRequested()
{
	return BWindow::QuitRequested();
}


void PincodeWindow::SetBDaddr(BString address)
{
	BString label;

	label << BD_ADDR_LABEL << address;
	printf("++ %s\n",label.String());
	fRemoteInfo->SetText(label.String());
	fRemoteInfo->ResizeToPreferred();
	//Invalidate();

}


} /* end namespace Bluetooth */
