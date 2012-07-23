/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "SerialApp.h"

#include "SerialWindow.h"


SerialApp::SerialApp()
	:
	BApplication(SerialApp::kApplicationSignature)
{
	fWindow = new SerialWindow();

	serialLock = create_sem(0, "Serial port lock");
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
			serialPort.Open(portName);
			release_sem(serialLock);
			break;
		}
		case kMsgDataRead:
		{
			// TODO forward to the window	
			break;
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

		bytesRead = application->serialPort.Read(buffer, 256);
		if (bytesRead == B_FILE_ERROR)
		{
			// Port is not open - wait for it and start over
			acquire_sem(application->serialLock);
		} else {
			// We read something, forward it to the app for handling
			BMessage* serialData = new BMessage(kMsgDataRead);
			// TODO bytesRead is not nul terminated. Use generic data rather
			serialData->AddString("data", buffer);
			be_app_messenger.SendMessage(serialData);
		}
	}
}

const char* SerialApp::kApplicationSignature
	= "application/x-vnd.haiku.SerialConnect";


int main(int argc, char** argv)
{
	SerialApp app;
	app.Run();
}
