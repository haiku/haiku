/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <stdio.h>

#include "SerialApp.h"

#include "SerialWindow.h"


SerialApp::SerialApp()
	:
	BApplication(SerialApp::kApplicationSignature)
{
	fWindow = new SerialWindow();

	fSerialLock = create_sem(0, "Serial port lock");
	thread_id id = spawn_thread(PollSerial, "Serial port poller",
		B_LOW_PRIORITY, this);
	resume_thread(id);
}


void SerialApp::ReadyToRun()
{
	fWindow->Show();
}


void SerialApp::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgOpenPort:
		{
			const char* portName;
			message->FindString("port name", &portName);
			fSerialPort.Open(portName);
			release_sem(fSerialLock);
			break;
		}
		case kMsgDataRead:
		{
			// forward the message to the window, which will display the
			// incoming data
			fWindow->PostMessage(message);
			break;
		}
		case kMsgDataWrite:
		{
			const char* bytes;
			ssize_t size;

			message->FindData("data", B_RAW_TYPE, (const void**)&bytes, &size);
			fSerialPort.Write(bytes, size);
		}
		default:
			BApplication::MessageReceived(message);
	}
}


/* static */
status_t SerialApp::PollSerial(void*)
{
	SerialApp* application = (SerialApp*)be_app;
	char buffer[256];

	for(;;)
	{
		ssize_t bytesRead;

		bytesRead = application->fSerialPort.Read(buffer, 256);
		if (bytesRead == B_FILE_ERROR)
		{
			// Port is not open - wait for it and start over
			acquire_sem(application->fSerialLock);
		} else if (bytesRead > 0) {
			// We read something, forward it to the app for handling
			BMessage* serialData = new BMessage(kMsgDataRead);
			serialData->AddData("data", B_RAW_TYPE, buffer, bytesRead);
			be_app_messenger.SendMessage(serialData);
		}
	}
	
	// Should not reach this line anyway...
	return B_OK;
}

const char* SerialApp::kApplicationSignature
	= "application/x-vnd.haiku.SerialConnect";


int main(int argc, char** argv)
{
	SerialApp app;
	app.Run();
}
