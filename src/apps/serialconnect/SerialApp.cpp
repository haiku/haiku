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
	fWindow->Show();
}


void SerialApp::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgOpenPort:
		{
			const char* portName;
			if(message->FindString("port name", &portName) == B_OK)
			{
				fSerialPort.Open(portName);
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

			message->FindData("data", B_RAW_TYPE, (const void**)&bytes, &size);

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
				data_rate rate;
				switch(baudrate) {
					case 50:
						rate = B_50_BPS;
						break;
					case 75:
						rate = B_75_BPS;
						break;
					case 110:
						rate = B_110_BPS;
						break;
					case 134:
						rate = B_134_BPS;
						break;
					case 150:
						rate = B_150_BPS;
						break;
					case 200:
						rate = B_200_BPS;
						break;
					case 300:
						rate = B_300_BPS;
						break;
					case 600:
						rate = B_600_BPS;
						break;
					case 1200:
						rate = B_1200_BPS;
						break;
					case 1800:
						rate = B_1800_BPS;
						break;
					case 2400:
						rate = B_2400_BPS;
						break;
					case 4800:
						rate = B_4800_BPS;
						break;
					case 9600:
						rate = B_9600_BPS;
						break;
					case 19200:
						rate = B_19200_BPS;
						break;
					case 31250:
						rate = B_31250_BPS;
						break;
					case 38400:
						rate = B_38400_BPS;
						break;
					case 57600:
						rate = B_57600_BPS;
						break;
					case 115200:
						rate = B_115200_BPS;
						break;
					case 230400:
						rate = B_230400_BPS;
						break;
					default:
						rate = B_0_BPS;
						break;
				}
				fSerialPort.SetDataRate(rate);
			}

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
