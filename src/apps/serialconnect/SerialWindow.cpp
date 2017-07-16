/*
 * Copyright 2012-2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT licence.
 */


#include "SerialWindow.h"

#include <stdio.h>

#include <FilePanel.h>
#include <GroupLayout.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <SerialPort.h>
#include <StatusBar.h>

#include "SerialApp.h"
#include "TermView.h"


const int SerialWindow::kBaudrates[] = { 50, 75, 110, 134, 150, 200, 300, 600,
	1200, 1800, 2400, 4800, 9600, 19200, 31250, 38400, 57600, 115200, 230400
};


// The values for these constants are not in the expected order, so we have to
// rely on this lookup table if we want to keep the menu items sorted.
const int SerialWindow::kBaudrateConstants[] = { B_50_BPS, B_75_BPS, B_110_BPS,
	B_134_BPS, B_150_BPS, B_200_BPS, B_300_BPS, B_600_BPS, B_1200_BPS,
	B_1800_BPS, B_2400_BPS, B_4800_BPS, B_9600_BPS, B_19200_BPS, B_31250_BPS,
	B_38400_BPS, B_57600_BPS, B_115200_BPS, B_230400_BPS
};


const char* SerialWindow::kWindowTitle = "SerialConnect";


SerialWindow::SerialWindow()
	: BWindow(BRect(100, 100, 400, 400), SerialWindow::kWindowTitle,
		B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
	, fLogFilePanel(NULL)
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "menuBar");
	menuBar->ResizeToPreferred();

	BRect r = Bounds();
	r.top = menuBar->Bounds().bottom + 1;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	fTermView = new TermView(r);
	fTermView->ResizeToPreferred();

	r = fTermView->Frame();
	r.left = r.right + 1;
	r.right = r.left + B_V_SCROLL_BAR_WIDTH;
	r.top -= 1;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT - 1;
	BScrollBar* scrollBar = new BScrollBar(r, "scrollbar", NULL, 0, 0,
		B_VERTICAL);

	scrollBar->SetTarget(fTermView);

	ResizeTo(r.right - 1, r.bottom + B_H_SCROLL_BAR_HEIGHT - 1);

	r = fTermView->Frame();
	r.top = r.bottom - 37;

	fStatusBar = new BStatusBar(r, "file transfer progress", NULL, NULL);
	fStatusBar->SetResizingMode(B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	fStatusBar->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fStatusBar->Hide();

	AddChild(menuBar);
	AddChild(fTermView);
	AddChild(scrollBar);
	AddChild(fStatusBar);

	fConnectionMenu = new BMenu("Connection");
	fFileMenu = new BMenu("File");
	BMenu* settingsMenu = new BMenu("Settings");

	fConnectionMenu->SetRadioMode(true);

	menuBar->AddItem(fConnectionMenu);
	menuBar->AddItem(fFileMenu);
	menuBar->AddItem(settingsMenu);

	// TODO edit menu - what's in it ?
	//BMenu* editMenu = new BMenu("Edit");
	//menuBar->AddItem(editMenu);

	BMenuItem* logFile = new BMenuItem("Log to file" B_UTF8_ELLIPSIS,
		new BMessage(kMsgLogfile));
	fFileMenu->AddItem(logFile);
	// TODO implement these
	BMenuItem* xmodemSend = new BMenuItem("XModem send" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSendXmodem));
	fFileMenu->AddItem(xmodemSend);
	xmodemSend->SetEnabled(false);
#if 0
	BMenuItem* xmodemReceive = new BMenuItem(
		"X/Y/Zmodem receive" B_UTF8_ELLIPSIS, NULL);
	fFileMenu->AddItem(xmodemReceive);
	xmodemReceive->SetEnabled(false);
#endif

	// Configuring all this by menus may be a bit unhandy. Make a setting
	// window instead ?
	fBaudrateMenu = new BMenu("Baud rate");
	fBaudrateMenu->SetRadioMode(true);
	settingsMenu->AddItem(fBaudrateMenu);

	fParityMenu = new BMenu("Parity");
	fParityMenu->SetRadioMode(true);
	settingsMenu->AddItem(fParityMenu);

	fStopbitsMenu = new BMenu("Stop bits");
	fStopbitsMenu->SetRadioMode(true);
	settingsMenu->AddItem(fStopbitsMenu);

	fFlowcontrolMenu = new BMenu("Flow control");
	fFlowcontrolMenu->SetRadioMode(true);
	settingsMenu->AddItem(fFlowcontrolMenu);

	fDatabitsMenu = new BMenu("Data bits");
	fDatabitsMenu->SetRadioMode(true);
	settingsMenu->AddItem(fDatabitsMenu);

	fLineTerminatorMenu = new BMenu("Line terminator");
	fLineTerminatorMenu->SetRadioMode(true);
	settingsMenu->AddItem(fLineTerminatorMenu);

	BMessage* message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_NO_PARITY);
	BMenuItem* parityNone = new BMenuItem("None", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_ODD_PARITY);
	BMenuItem* parityOdd = new BMenuItem("Odd", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_EVEN_PARITY);
	BMenuItem* parityEven = new BMenuItem("Even", message);

	fParityMenu->AddItem(parityNone);
	fParityMenu->AddItem(parityOdd);
	fParityMenu->AddItem(parityEven);
	fParityMenu->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("databits", B_DATA_BITS_7);
	BMenuItem* data7 = new BMenuItem("7", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("databits", B_DATA_BITS_8);
	BMenuItem* data8 = new BMenuItem("8", message);

	fDatabitsMenu->AddItem(data7);
	fDatabitsMenu->AddItem(data8);
	fDatabitsMenu->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("stopbits", B_STOP_BITS_1);
	BMenuItem* stop1 = new BMenuItem("1", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("stopbits", B_STOP_BITS_2);
	BMenuItem* stop2 = new BMenuItem("2", message);

	fStopbitsMenu->AddItem(stop1);
	fStopbitsMenu->AddItem(stop2);
	fStopbitsMenu->SetTargetForItems(be_app);

	// Loop backwards to add fastest rates at top of menu
	for (int i = sizeof(kBaudrates) / sizeof(kBaudrates[0]); --i >= 0;)
	{
		message = new BMessage(kMsgSettings);
		message->AddInt32("baudrate", kBaudrateConstants[i]);

		char buffer[7];
		sprintf(buffer, "%d", kBaudrates[i]);
		BMenuItem* item = new BMenuItem(buffer, message);

		fBaudrateMenu->AddItem(item);
	}

	message = new BMessage(kMsgCustomBaudrate);
	BMenuItem* custom = new BMenuItem("custom" B_UTF8_ELLIPSIS, message);
	fBaudrateMenu->AddItem(custom);

	fBaudrateMenu->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_HARDWARE_CONTROL);
	BMenuItem* hardware = new BMenuItem("Hardware", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_SOFTWARE_CONTROL);
	BMenuItem* software = new BMenuItem("Software", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_HARDWARE_CONTROL | B_SOFTWARE_CONTROL);
	BMenuItem* both = new BMenuItem("Both", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", 0);
	BMenuItem* noFlow = new BMenuItem("None", message);

	fFlowcontrolMenu->AddItem(hardware);
	fFlowcontrolMenu->AddItem(software);
	fFlowcontrolMenu->AddItem(both);
	fFlowcontrolMenu->AddItem(noFlow);
	fFlowcontrolMenu->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddString("terminator", "\n");
	BMenuItem* lf = new BMenuItem("LF (\\n)", message);

	message = new BMessage(kMsgSettings);
	message->AddString("terminator", "\r");
	BMenuItem* cr = new BMenuItem("CR (\\r)", message);

	message = new BMessage(kMsgSettings);
	message->AddString("terminator", "\r\n");
	BMenuItem* crlf = new BMenuItem("CR/LF (\\r\\n)", message);

	fLineTerminatorMenu->AddItem(lf);
	fLineTerminatorMenu->AddItem(cr);
	fLineTerminatorMenu->AddItem(crlf);

	CenterOnScreen();
}


SerialWindow::~SerialWindow()
{
	delete fLogFilePanel;
}


void SerialWindow::MenusBeginning()
{
	// remove all items from the menu
	fConnectionMenu->RemoveItems(0, fConnectionMenu->CountItems(), true);

	// fill it with the (updated) serial port list
	BSerialPort serialPort;
	int deviceCount = serialPort.CountDevices();
	bool connected = false;

	for (int i = 0; i < deviceCount; i++)
	{
		char buffer[256];
		serialPort.GetDeviceName(i, buffer, 256);

		BMessage* message = new BMessage(kMsgOpenPort);
		message->AddString("port name", buffer);
		BMenuItem* portItem = new BMenuItem(buffer, message);
		portItem->SetTarget(be_app);

		const BString& connectedPort = ((SerialApp*)be_app)->GetPort();

		if (connectedPort == buffer) {
			connected = true;
			portItem->SetMarked(true);
		}

		fConnectionMenu->AddItem(portItem);
	}

	if (deviceCount > 0) {
		fConnectionMenu->AddSeparatorItem();

		BMenuItem* disconnect = new BMenuItem("Disconnect",
			new BMessage(kMsgOpenPort), 'Z', B_OPTION_KEY);
		if (!connected)
			disconnect->SetEnabled(false);
		disconnect->SetTarget(be_app);
		fConnectionMenu->AddItem(disconnect);
	} else {
		BMenuItem* noDevices = new BMenuItem("<no serial port available>", NULL);
		noDevices->SetEnabled(false);
		fConnectionMenu->AddItem(noDevices);
	}
}


void SerialWindow::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case kMsgOpenPort:
		{
			BString path;
			bool open = (message->FindString("port name", &path) == B_OK);
			int i = 1; // Skip "log to file", which woeks even when offline.
			BMenuItem* item;
			while((item = fFileMenu->ItemAt(i++)))
			{
				item->SetEnabled(open);
			}
			return;
		}
		case kMsgDataRead:
		{
			const char* bytes;
			ssize_t length;
			if (message->FindData("data", B_RAW_TYPE, (const void**)&bytes,
					&length) == B_OK)
				fTermView->PushBytes(bytes, length);
			return;
		}
		case kMsgLogfile:
		case kMsgSendXmodem:
		{
			// Let's lazy init the file panel
			if (fLogFilePanel == NULL) {
				fLogFilePanel = new BFilePanel(
					message->what == kMsgSendXmodem ? B_OPEN_PANEL : B_SAVE_PANEL,
					&be_app_messenger, NULL, B_FILE_NODE, false);
				fLogFilePanel->SetMessage(message);
			}
			fLogFilePanel->Show();
			return;
		}
		case kMsgSettings:
		{
			int32 baudrate;
			stop_bits stopBits;
			data_bits dataBits;
			parity_mode parity;
			uint32 flowcontrol;
			BString terminator;

			if (message->FindInt32("databits", (int32*)&dataBits) == B_OK) {
				for (int i = 0; i < fDatabitsMenu->CountItems(); i++) {
					BMenuItem* item = fDatabitsMenu->ItemAt(i);
					int32 code;
					item->Message()->FindInt32("databits", &code);

					if (code == dataBits)
						item->SetMarked(true);
				}
			}

			if (message->FindInt32("stopbits", (int32*)&stopBits) == B_OK) {
				for (int i = 0; i < fStopbitsMenu->CountItems(); i++) {
					BMenuItem* item = fStopbitsMenu->ItemAt(i);
					int32 code;
					item->Message()->FindInt32("stopbits", &code);

					if (code == stopBits)
						item->SetMarked(true);
				}
			}

			if (message->FindInt32("parity", (int32*)&parity) == B_OK)
			{
				for (int i = 0; i < fParityMenu->CountItems(); i++) {
					BMenuItem* item = fParityMenu->ItemAt(i);
					int32 code;
					item->Message()->FindInt32("parity", &code);

					if (code == parity)
						item->SetMarked(true);
				}
			}

			if (message->FindInt32("flowcontrol", (int32*)&flowcontrol)
					== B_OK) {
				for (int i = 0; i < fFlowcontrolMenu->CountItems(); i++) {
					BMenuItem* item = fFlowcontrolMenu->ItemAt(i);
					int32 code;
					item->Message()->FindInt32("flowcontrol", &code);

					if (code == (int32)flowcontrol)
						item->SetMarked(true);
				}
			}

			if (message->FindInt32("baudrate", &baudrate) == B_OK) {
				int i;
				BMenuItem* item = NULL;
				for (i = 0; i < fBaudrateMenu->CountItems(); i++) {
					item = fBaudrateMenu->ItemAt(i);
					int32 code = 0;
					item->Message()->FindInt32("baudrate", &code);

					if (baudrate == code) {
						item->SetMarked(true);
						break;
					}
				}

				if (i == fBaudrateMenu->CountItems() && item != NULL) {
					// Rate was not found, mark it as "custom".
					// Since that is the last item in the menu, we still point
					// to it.
					item->SetMarked(true);
					item->Message()->SetInt32("baudrate", baudrate);
				}
			}

			if (message->FindString("terminator", &terminator) == B_OK) {
				fTermView->SetLineTerminator(terminator);
				for (int i = 0; i < fLineTerminatorMenu->CountItems(); i++) {
					BMenuItem* item = fLineTerminatorMenu->ItemAt(i);
					BString code;
					item->Message()->FindString("terminator", &code);

					if (terminator == code)
						item->SetMarked(true);
				}
			}

			return;
		}
		case kMsgProgress:
		{
			// File transfer progress
			int32 pos = message->FindInt32("pos");
			int32 size = message->FindInt32("size");
			BString label = message->FindString("info");

			if (pos >= size) {
				if (!fStatusBar->IsHidden()) {
					fStatusBar->Hide();
					fTermView->ResizeBy(0, fStatusBar->Bounds().Height() - 1);
				}
			} else {
				BString text;
				text.SetToFormat("%" B_PRId32 "/%" B_PRId32, pos, size);
				fStatusBar->SetMaxValue(size);
				fStatusBar->SetTo(pos, label, text);
				if (fStatusBar->IsHidden()) {
					fStatusBar->Show();
					fTermView->ResizeBy(0, -(fStatusBar->Bounds().Height() - 1));
				}
			}
			return;
		}
	}

	BWindow::MessageReceived(message);
}
