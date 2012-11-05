/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "SerialWindow.h"

#include <stdio.h>

#include <FilePanel.h>
#include <GroupLayout.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <SerialPort.h>

#include "SerialApp.h"
#include "TermView.h"


SerialWindow::SerialWindow()
	: BWindow(BRect(100, 100, 400, 400), SerialWindow::kWindowTitle,
		B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
	, fLogFilePanel(NULL)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0.0f));

	BMenuBar* menuBar = new BMenuBar("menuBar");
	fTermView = new TermView();

	AddChild(menuBar);
	AddChild(fTermView);

	fConnectionMenu = new BMenu("Connection");
	BMenu* fileMenu = new BMenu("File");
	BMenu* settingsMenu = new BMenu("Settings");

	fConnectionMenu->SetRadioMode(true);

	menuBar->AddItem(fConnectionMenu);
	menuBar->AddItem(fileMenu);
	menuBar->AddItem(settingsMenu);

	// TODO edit menu - what's in it ?
	//BMenu* editMenu = new BMenu("Edit");
	//menuBar->AddItem(editMenu);
	
	BMenuItem* logFile = new BMenuItem("Log to file" B_UTF8_ELLIPSIS,
		new BMessage(kMsgLogfile));
	fileMenu->AddItem(logFile);
	BMenuItem* xmodemSend = new BMenuItem("X/Y/ZModem send" B_UTF8_ELLIPSIS,
		NULL);
	fileMenu->AddItem(xmodemSend);
	BMenuItem* xmodemReceive = new BMenuItem(
		"X/Y/Zmodem receive" B_UTF8_ELLIPSIS, NULL);
	fileMenu->AddItem(xmodemReceive);

	// Configuring all this by menus may be a bit unhandy. Make a setting
	// window instead ?
	BMenu* baudRate = new BMenu("Baud rate"); 
	baudRate->SetRadioMode(true);
	settingsMenu->AddItem(baudRate);

	BMenu* parity = new BMenu("Parity"); 
	parity->SetRadioMode(true);
	settingsMenu->AddItem(parity);

	BMenu* stopBits = new BMenu("Stop bits"); 
	stopBits->SetRadioMode(true);
	settingsMenu->AddItem(stopBits);

	BMenu* flowControl = new BMenu("Flow control"); 
	flowControl->SetRadioMode(true);
	settingsMenu->AddItem(flowControl);

	BMenu* dataBits = new BMenu("Data bits"); 
	dataBits->SetRadioMode(true);
	settingsMenu->AddItem(dataBits);


	BMessage* message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_NO_PARITY);
	BMenuItem* parityNone = new BMenuItem("None", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_ODD_PARITY);
	BMenuItem* parityOdd = new BMenuItem("Odd", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("parity", B_EVEN_PARITY);
	BMenuItem* parityEven = new BMenuItem("Even", message);
	parityNone->SetMarked(true);

	parity->AddItem(parityNone);
	parity->AddItem(parityOdd);
	parity->AddItem(parityEven);
	parity->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("databits", B_DATA_BITS_7);
	BMenuItem* data7 = new BMenuItem("7", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("databits", B_DATA_BITS_8);
	BMenuItem* data8 = new BMenuItem("8", message);
	data8->SetMarked(true);

	dataBits->AddItem(data7);
	dataBits->AddItem(data8);
	dataBits->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("stopbits", B_STOP_BITS_1);
	BMenuItem* stop1 = new BMenuItem("1", NULL);

	message = new BMessage(kMsgSettings);
	message->AddInt32("stopbits", B_STOP_BITS_2);
	BMenuItem* stop2 = new BMenuItem("2", NULL);
	stop1->SetMarked(true);

	stopBits->AddItem(stop1);
	stopBits->AddItem(stop2);
	stopBits->SetTargetForItems(be_app);

	static const int baudrates[] = { 50, 75, 110, 134, 150, 200, 300, 600,
		1200, 1800, 2400, 4800, 9600, 19200, 31250, 38400, 57600, 115200,
		230400
	};

	// Loop backwards to add fastest rates at top of menu
	for (int i = sizeof(baudrates) / sizeof(char*); --i >= 0;)
	{
		message = new BMessage(kMsgSettings);
		message->AddInt32("baudrate", baudrates[i]);

		char buffer[7];
		sprintf(buffer,"%d", baudrates[i]);
		BMenuItem* item = new BMenuItem(buffer, message);

		if (baudrates[i] == 19200)
			item->SetMarked(true);

		baudRate->AddItem(item);
	}

	baudRate->SetTargetForItems(be_app);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_HARDWARE_CONTROL);
	BMenuItem* hardware = new BMenuItem("Hardware", message);
	hardware->SetMarked(true);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_SOFTWARE_CONTROL);
	BMenuItem* software = new BMenuItem("Software", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", B_HARDWARE_CONTROL | B_SOFTWARE_CONTROL);
	BMenuItem* both = new BMenuItem("Both", message);

	message = new BMessage(kMsgSettings);
	message->AddInt32("flowcontrol", 0);
	BMenuItem* noFlow = new BMenuItem("None", message);

	flowControl->AddItem(hardware);
	flowControl->AddItem(software);
	flowControl->AddItem(both);
	flowControl->AddItem(noFlow);
	flowControl->SetTargetForItems(be_app);

	CenterOnScreen();
}



SerialWindow::~SerialWindow()
{
	delete fLogFilePanel;
}


void SerialWindow::MenusBeginning()
{
	// remove all items from the menu
	while(fConnectionMenu->RemoveItem(0L));

	// fill it with the (updated) serial port list
	BSerialPort serialPort;
	int deviceCount = serialPort.CountDevices();

	for(int i = 0; i < deviceCount; i++)
	{
		char buffer[256];
		serialPort.GetDeviceName(i, buffer, 256);

		BMessage* message = new BMessage(kMsgOpenPort);
		message->AddString("port name", buffer);
		BMenuItem* portItem = new BMenuItem(buffer, message);
		portItem->SetTarget(be_app);

		fConnectionMenu->AddItem(portItem);
	}

	if (deviceCount > 0) {
		fConnectionMenu->AddSeparatorItem();

		BMenuItem* disconnect = new BMenuItem("Disconnect",
			new BMessage(kMsgOpenPort), 'Z', B_OPTION_KEY);
		fConnectionMenu->AddItem(disconnect);
	} else {
		BMenuItem* noDevices = new BMenuItem("<no serial port available>", NULL);
		noDevices->SetEnabled(false);
		fConnectionMenu->AddItem(noDevices);
	}
}

void SerialWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgOpenPort:
		{
			BMenuItem* disconnectMenu;
			if(message->FindPointer("source", (void**)&disconnectMenu) == B_OK)
				disconnectMenu->SetMarked(false);
			be_app->PostMessage(new BMessage(*message));
			break;
		}
		case kMsgDataRead:
		{
			const char* bytes;
			ssize_t length;
			message->FindData("data", B_RAW_TYPE, (const void**)&bytes, &length);
			fTermView->PushBytes(bytes, length);
			break;
		}
		case kMsgLogfile:
		{
			// Let's lazy init the file panel
			if(fLogFilePanel == NULL) {
				fLogFilePanel = new BFilePanel(B_SAVE_PANEL, &be_app_messenger,
					NULL, B_FILE_NODE, false);
				fLogFilePanel->SetMessage(message);
			}
			fLogFilePanel->Show();
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}


const char* SerialWindow::kWindowTitle = "SerialConnect";
