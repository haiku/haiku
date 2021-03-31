/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
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


namespace Bluetooth
{

PincodeWindow::PincodeWindow(bdaddr_t address, hci_id hid)
	: BWindow(BRect(700, 200, 1000, 400), "PIN Code Request",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
		fBdaddr(address),
		fHid(hid)
{
	InitUI();

	// TODO: Get more info about device" ote name/features/encry/auth... etc
	SetBDaddr(bdaddrUtils::ToString(fBdaddr));

}


PincodeWindow::PincodeWindow(RemoteDevice* rDevice)
	: BWindow(BRect(700, 200, 1000, 400), "PIN Code Request",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	InitUI();

	// TODO: Get more info about device" ote name/features/encry/auth... etc
	SetBDaddr(bdaddrUtils::ToString(rDevice->GetBluetoothAddress()));
	fHid = (rDevice->GetLocalDeviceOwner())->ID();
}


void
PincodeWindow::InitUI()
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fIcon = new BluetoothIconView();

	fMessage = new BStringView("fMessage", "Input the PIN code to pair with");
	fMessage2 = new BStringView("fMessage2", "the following Bluetooth device.");

	fDeviceLabel = new BStringView("fDeviceLabel","Device Name: ");
	fDeviceLabel->SetFont(be_bold_font);

	fDeviceText = new BStringView("fDeviceText", "<unknown_device>");

	fAddressLabel = new BStringView("fAddressLabel", "MAC Address: ");
	fAddressLabel->SetFont(be_bold_font);

	fAddressText = new BStringView("fAddressText", "<mac_address>");

	fPincodeText = new BTextControl("fPINCode", "PIN Code:", "0000", NULL);
	fPincodeText->TextView()->SetMaxBytes(16 * sizeof(fPincodeText->Text()[0]));
	fPincodeText->MakeFocus();

	fAcceptButton = new BButton("fAcceptButton", "Pair",
		new BMessage(skMessageAcceptButton));

	fCancelButton = new BButton("fCancelButton", "Cancel",
		new BMessage(skMessageCancelButton));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 8)
				.Add(fIcon)
			)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
				.Add(fMessage)
				.Add(fMessage2)
				.AddGlue()
			)
		)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(fDeviceLabel)
			.AddGlue()
			.Add(fDeviceText)
		)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(fAddressLabel)
			.AddGlue()
			.Add(fAddressText)
		)
		.AddGlue()
		.Add(fPincodeText)
		.AddGlue()
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fAcceptButton)
		)
		.SetInsets(8, 8, 8, 8)
	);
}


void
PincodeWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what)
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
				if (reply.FindInt8("status", &bt_status ) == B_OK) {
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


bool
PincodeWindow::QuitRequested()
{
	return BWindow::QuitRequested();
}


void
PincodeWindow::SetBDaddr(BString address)
{
	fAddressText->SetText(address);
}

} /* end namespace Bluetooth */
