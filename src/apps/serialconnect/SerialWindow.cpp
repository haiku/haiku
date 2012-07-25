/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "SerialWindow.h"

#include <GroupLayout.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <SerialPort.h>

#include "SerialApp.h"
#include "TermView.h"


SerialWindow::SerialWindow()
	:
	BWindow(BRect(100, 100, 400, 400), SerialWindow::kWindowTitle,
		B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0.0f));

	BMenuBar* menuBar = new BMenuBar("menuBar");
	fTermView = new TermView();

	AddChild(menuBar);
	AddChild(fTermView);

	BMenu* connectionMenu = new BMenu("Connections");
	BMenu* editMenu = new BMenu("Edit");
	BMenu* settingsMenu = new BMenu("Settings");

	menuBar->AddItem(connectionMenu);
	menuBar->AddItem(editMenu);
	menuBar->AddItem(settingsMenu);

	// TODO messages
	BMenu* connect = new BMenu("Connect");
	connectionMenu->AddItem(connect);

	BSerialPort serialPort;
	int deviceCount = serialPort.CountDevices();

	for(int i = 0; i < deviceCount; i++)
	{
		char buffer[256];
		serialPort.GetDeviceName(i, buffer, 256);

		BMessage* message = new BMessage(kMsgOpenPort);
		message->AddString("port name", buffer);
		BMenuItem* portItem = new BMenuItem(buffer, message);

		connect->AddItem(portItem);
	}

#if SUPPORTS_MODEM
	BMenuItem* connectModem = new BMenuItem(
		"Connect via modem" B_UTF8_ELLIPSIS, NULL, 'M', 0);
	connectionMenu->AddItem(connectModem);
#endif
	BMenuItem* Disconnect = new BMenuItem("Disconnect", NULL,
		'Z', B_OPTION_KEY);
	connectionMenu->AddItem(Disconnect);

	// TODO edit menu - what's in it ?
	
	// Configuring all this by menus may be a bit unhandy. Make a setting
	// window instead ?
	BMenu* parity = new BMenu("Parity"); 
	settingsMenu->AddItem(parity);
	BMenu* dataBits = new BMenu("Data bits"); 
	settingsMenu->AddItem(dataBits);
	BMenu* stopBits = new BMenu("Stop bits"); 
	settingsMenu->AddItem(stopBits);
	BMenu* baudRate = new BMenu("Baud rate"); 
	settingsMenu->AddItem(baudRate);
	BMenu* flowControl = new BMenu("Flow control"); 
	settingsMenu->AddItem(flowControl);

	BMenuItem* parityNone = new BMenuItem("None", NULL);
	parity->AddItem(parityNone);
	BMenuItem* parityOdd = new BMenuItem("Odd", NULL);
	parity->AddItem(parityOdd);
	BMenuItem* parityEven = new BMenuItem("Even", NULL);
	parity->AddItem(parityEven);

	BMenuItem* data7 = new BMenuItem("7", NULL);
	dataBits->AddItem(data7);
	BMenuItem* data8 = new BMenuItem("8", NULL);
	dataBits->AddItem(data8);

	BMenuItem* stop1 = new BMenuItem("1", NULL);
	stopBits->AddItem(stop1);
	BMenuItem* stop2 = new BMenuItem("2", NULL);
	stopBits->AddItem(stop2);

	static const char* baudrates[] = { "50", "75", "110", "134", "150", "200",
		"300", "600", "1200", "1800", "2400", "4800", "9600", "19200", "31250",
		"38400", "57600", "115200", "230400"
	};

	// Loop backwards to add fastest rates at top of menu
	for (int i = sizeof(baudrates) / sizeof(char*); --i >= 0;)
	{
		BMenuItem* item = new BMenuItem(baudrates[i], NULL);
		baudRate->AddItem(item);
	}

	BMenuItem* rtsCts = new BMenuItem("RTS/CTS", NULL);
	flowControl->AddItem(rtsCts);
	BMenuItem* noFlow = new BMenuItem("None", NULL);
	flowControl->AddItem(noFlow);

	CenterOnScreen();
}


void SerialWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgDataRead:
		{
			const char* bytes;
			ssize_t length;
			message->FindData("data", B_RAW_TYPE, &(const void*)bytes, &length);
			fTermView->PushBytes(bytes, length);
			break;
		}
		case kMsgOpenPort:
		{
			// Forward message to application
			be_app->PostMessage(message);
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}


const char* SerialWindow::kWindowTitle = "SerialConnect";
