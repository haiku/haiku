/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "SerialApp.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include "SerialWindow.h"


SerialApp::SerialApp()
	: BApplication(SerialApp::kApplicationSignature)
	, fLogFile(NULL)
{
	fWindow = new SerialWindow();

	fSerialLock = create_sem(0, "Serial port lock");
	thread_id id = spawn_thread(PollSerial, "Serial port poller",
		B_LOW_PRIORITY, this);
	resume_thread(id);
}


SerialApp::~SerialApp()
{
	delete fLogFile;
}


void SerialApp::ReadyToRun()
{
	LoadSettings();
	fWindow->Show();
}


void SerialApp::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgOpenPort:
		{
			if(message->FindString("port name", &fPortPath) == B_OK)
			{
				fSerialPort.Open(fPortPath);
				release_sem(fSerialLock);
			} else {
				fSerialPort.Close();
			}
			break;
		}
		case kMsgDataRead:
		{
			// forward the message to the window, which will display the
			// incoming data
			fWindow->PostMessage(message);

			if (fLogFile)
			{
				const char* bytes;
				ssize_t length;
				message->FindData("data", B_RAW_TYPE, (const void**)&bytes,
					&length);
				if(fLogFile->Write(bytes, length) != length)
				{
					// TODO error handling
				}
			}

			break;
		}
		case kMsgDataWrite:
		{
			const char* bytes;
			ssize_t size;

			if (message->FindData("data", B_RAW_TYPE, (const void**)&bytes,
					&size) != B_OK)
				break;

			if (bytes[0] == '\n') {
				size = 2;
				bytes = "\r\n";
			}
			fSerialPort.Write(bytes, size);
			break;
		}
		case kMsgLogfile:
		{
			entry_ref parent;
			const char* filename;

			if (message->FindRef("directory", &parent) == B_OK
				&& message->FindString("name", &filename) == B_OK)
			{
				delete fLogFile;
				BDirectory directory(&parent);
				fLogFile = new BFile(&directory, filename,
					B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
				status_t error = fLogFile->InitCheck();
				if(error != B_OK)
				{
					puts(strerror(error));
				}
			} else {
				debugger("Invalid BMessage received");
			}
			break;
		}
		case kMsgSettings:
		{
			int32 baudrate;
			stop_bits stopBits;
			data_bits dataBits;
			parity_mode parity;
			uint32 flowcontrol;

			if(message->FindInt32("databits", (int32*)&dataBits) == B_OK)
				fSerialPort.SetDataBits(dataBits);

			if(message->FindInt32("stopbits", (int32*)&stopBits) == B_OK)
				fSerialPort.SetStopBits(stopBits);

			if(message->FindInt32("parity", (int32*)&parity) == B_OK)
				fSerialPort.SetParityMode(parity);

			if(message->FindInt32("flowcontrol", (int32*)&flowcontrol) == B_OK)
				fSerialPort.SetFlowControl(flowcontrol);

			if(message->FindInt32("baudrate", &baudrate) == B_OK) {
				data_rate rate = (data_rate)baudrate;
				fSerialPort.SetDataRate(rate);
			}

			break;
		}
		default:
			BApplication::MessageReceived(message);
	}
}


bool SerialApp::QuitRequested()
{
	if(BApplication::QuitRequested()) {
		SaveSettings();
		return true;
	}
	return false;
}


const BString& SerialApp::GetPort()
{
	return fPortPath;
}


void SerialApp::LoadSettings()
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("SerialConnect");

	BFile file(path.Path(), B_READ_ONLY);
	BMessage message(kMsgSettings);
	if(message.Unflatten(&file) != B_OK)
	{
		message.AddInt32("parity", fSerialPort.ParityMode());
		message.AddInt32("databits", fSerialPort.DataBits());
		message.AddInt32("stopbits", fSerialPort.StopBits());
		message.AddInt32("baudrate", fSerialPort.DataRate());
		message.AddInt32("flowcontrol", fSerialPort.FlowControl());
	}

	be_app->PostMessage(&message);
	fWindow->PostMessage(&message);
}


void SerialApp::SaveSettings()
{
	BMessage message(kMsgSettings);
	message.AddInt32("parity", fSerialPort.ParityMode());
	message.AddInt32("databits", fSerialPort.DataBits());
	message.AddInt32("stopbits", fSerialPort.StopBits());
	message.AddInt32("baudrate", fSerialPort.DataRate());
	message.AddInt32("flowcontrol", fSerialPort.FlowControl());

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("SerialConnect");

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	message.Flatten(&file);
}


/* static */
status_t SerialApp::PollSerial(void*)
{
	SerialApp* application = (SerialApp*)be_app;
	char buffer[256];

	for(;;)
	{
		ssize_t bytesRead;

		bytesRead = application->fSerialPort.Read(buffer, sizeof(buffer));
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
