/*
 * Copyright 2012-2017, Adrien Destugues, pulkomandy@gmail.com
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

#include "CustomRateWindow.h"
#include "SerialWindow.h"


static property_info sProperties[] = {
	{ "baudrate",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, B_DIRECT_SPECIFIER, 0 },
		"get or set the baudrate",
		0, { B_INT32_TYPE }
	},
	{ "bits",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, B_DIRECT_SPECIFIER, 0 },
		"get or set the number of data bits (7 or 8)",
		0, { B_INT32_TYPE }
	},
	{ "stopbits",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, B_DIRECT_SPECIFIER, 0 },
		"get or set the number of stop bits (1 or 2)",
		0, { B_INT32_TYPE }
	},
	{ "parity",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, B_DIRECT_SPECIFIER, 0 },
		"get or set the parity (none, even or odd)",
		0, { B_STRING_TYPE }
	},
	{ "flowcontrol",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, B_DIRECT_SPECIFIER, 0 },
		"get or set the flow control (hardware, software, both, or none)",
		0, { B_STRING_TYPE }
	},
	{ "port",
		{ B_GET_PROPERTY, B_SET_PROPERTY, B_DELETE_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"get or set the port device",
		0, { B_STRING_TYPE }
	},
	{ NULL }
};

const BPropertyInfo SerialApp::kScriptingProperties(sProperties);


SerialApp::SerialApp()
	: BApplication(SerialApp::kApplicationSignature)
	, fLogFile(NULL)
	, fFileSender(NULL)
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
	delete fFileSender;
}


void SerialApp::ReadyToRun()
{
	LoadSettings();
	fWindow->Show();
}


void SerialApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenPort:
		{
			if (message->FindString("port name", &fPortPath) == B_OK) {
				fSerialPort.Open(fPortPath);
				release_sem(fSerialLock);
			} else {
				fSerialPort.Close();
			}

			// Forward to the window so it can enable/disable menu items
			fWindow->PostMessage(message);
			return;
		}
		case kMsgDataRead:
		{
			const uint8_t* bytes;
			ssize_t length;
			message->FindData("data", B_RAW_TYPE, (const void**)&bytes,
				&length);

			if (fFileSender != NULL) {
				if (fFileSender->BytesReceived(bytes, length)) {
					delete fFileSender;
					fFileSender = NULL;
				}
			} else {
				// forward the message to the window, which will display the
				// incoming data
				fWindow->PostMessage(message);

				if (fLogFile) {
					if (fLogFile->Write(bytes, length) != length) {
						// TODO error handling
					}
				}
			}

			return;
		}
		case kMsgDataWrite:
		{
			// Do not allow sending if a file transfer is in progress.
			if (fFileSender != NULL)
				return;

			const char* bytes;
			ssize_t size;

			if (message->FindData("data", B_RAW_TYPE, (const void**)&bytes,
					&size) == B_OK)
				fSerialPort.Write(bytes, size);
			return;
		}
		case kMsgLogfile:
		{
			entry_ref parent;
			const char* filename;

			if (message->FindRef("directory", &parent) == B_OK
				&& message->FindString("name", &filename) == B_OK) {
				delete fLogFile;
				BDirectory directory(&parent);
				fLogFile = new BFile(&directory, filename,
					B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
				status_t error = fLogFile->InitCheck();
				if (error != B_OK)
					puts(strerror(error));
			} else
				debugger("Invalid BMessage received");
			return;
		}
		case kMsgSendXmodem:
		{
			entry_ref ref;

			if (message->FindRef("refs", &ref) == B_OK) {
				BFile* file = new BFile(&ref, B_READ_ONLY);
				status_t error = file->InitCheck();
				if (error != B_OK)
					puts(strerror(error));
				else {
					delete fFileSender;
					fFileSender = new XModemSender(file, &fSerialPort, fWindow);
				}
			} else {
				message->PrintToStream();
				debugger("Invalid BMessage received");
			}
			return;
		}
		case kMsgCustomBaudrate:
		{
			// open the custom baudrate selector window
			CustomRateWindow* window = new CustomRateWindow(fSerialPort.DataRate());
			window->Show();
			return;
		}
		case kMsgSettings:
		{
			int32 baudrate;
			stop_bits stopBits;
			data_bits dataBits;
			parity_mode parity;
			uint32 flowcontrol;

			if (message->FindInt32("databits", (int32*)&dataBits) == B_OK)
				fSerialPort.SetDataBits(dataBits);

			if (message->FindInt32("stopbits", (int32*)&stopBits) == B_OK)
				fSerialPort.SetStopBits(stopBits);

			if (message->FindInt32("parity", (int32*)&parity) == B_OK)
				fSerialPort.SetParityMode(parity);

			if (message->FindInt32("flowcontrol", (int32*)&flowcontrol) == B_OK)
				fSerialPort.SetFlowControl(flowcontrol);

			if (message->FindInt32("baudrate", &baudrate) == B_OK) {
				data_rate rate = (data_rate)baudrate;
				fSerialPort.SetDataRate(rate);
			}

			return;
		}
	}

	// Handle scripting messages
	if (message->HasSpecifiers()) {
		BMessage specifier;
		int32 what;
		int32 index;
		const char* property;

		BMessage reply(B_REPLY);
		BMessage settings(kMsgSettings);
		bool settingsChanged = false;

		if (message->GetCurrentSpecifier(&index, &specifier, &what, &property)
			== B_OK) {
			switch (kScriptingProperties.FindMatch(message, index, &specifier,
				what, property)) {
				case 0: // baudrate
					if (message->what == B_GET_PROPERTY) {
						reply.AddInt32("result", fSerialPort.DataRate());
						message->SendReply(&reply);
						return;
					}
					if (message->what == B_SET_PROPERTY) {
						int32 rate = message->FindInt32("data");
						settingsChanged = true;
						settings.AddInt32("baudrate", rate);
					}
					break;
				case 1: // data bits
					if (message->what == B_GET_PROPERTY) {
						reply.AddInt32("result", fSerialPort.DataBits() + 7);
						message->SendReply(&reply);
						return;
					}
					if (message->what == B_SET_PROPERTY) {
						int32 bits = message->FindInt32("data");
						settingsChanged = true;
						settings.AddInt32("databits", bits - 7);
					}
					break;
				case 2: // stop bits
					if (message->what == B_GET_PROPERTY) {
						reply.AddInt32("result", fSerialPort.StopBits() + 1);
						message->SendReply(&reply);
						return;
					}
					if (message->what == B_SET_PROPERTY) {
						int32 bits = message->FindInt32("data");
						settingsChanged = true;
						settings.AddInt32("stopbits", bits - 1);
					}
					break;
				case 3: // parity
				{
					static const char* strings[] = {"none", "odd", "even"};
					if (message->what == B_GET_PROPERTY) {
						reply.AddString("result",
							strings[fSerialPort.ParityMode()]);
						message->SendReply(&reply);
						return;
					}
					if (message->what == B_SET_PROPERTY) {
						BString bits = message->FindString("data");
						int i;
						for (i = 0; i < 3; i++) {
							if (bits == strings[i])
								break;
						}

						if (i < 3) {
							settingsChanged = true;
							settings.AddInt32("parity", i);
						}
					}
					break;
				}
				case 4: // flow control
				{
					static const char* strings[] = {"none", "hardware",
						"software", "both"};
					if (message->what == B_GET_PROPERTY) {
						reply.AddString("result",
							strings[fSerialPort.FlowControl()]);
						message->SendReply(&reply);
						return;
					}
					if (message->what == B_SET_PROPERTY) {
						BString bits = message->FindString("data");
						int i;
						for (i = 0; i < 4; i++) {
							if (bits == strings[i])
								break;
						}

						if (i < 4) {
							settingsChanged = true;
							settings.AddInt32("flowcontrol", i);
						}
					}
					break;
				}
				case 5: // port
					if (message->what == B_GET_PROPERTY) {
						reply.AddString("port", GetPort());
						message->SendReply(&reply);
					} else if (message->what == B_DELETE_PROPERTY
						|| message->what == B_SET_PROPERTY) {
						BString path = message->FindString("data");
						BMessage openMessage(kMsgOpenPort);
						openMessage.AddString("port name", path);
						PostMessage(&openMessage);
						fWindow->PostMessage(&openMessage);
					}
					return;
			}
		}

		if (settingsChanged) {
			PostMessage(&settings);
			fWindow->PostMessage(&settings);
			return;
		}
	}

	BApplication::MessageReceived(message);
}


bool SerialApp::QuitRequested()
{
	if (BApplication::QuitRequested()) {
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
	if (message.Unflatten(&file) != B_OK) {
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

	for (;;) {
		ssize_t bytesRead;

		bytesRead = application->fSerialPort.Read(buffer, sizeof(buffer));
		if (bytesRead == B_FILE_ERROR) {
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


status_t
SerialApp::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-SerialPort");
	message->AddFlat("messages", &kScriptingProperties);
	return BApplication::GetSupportedSuites(message);
}


BHandler*
SerialApp::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	if (kScriptingProperties.FindMatch(message, index, specifier, what,
		property) >= 0)
		return this;

	return BApplication::ResolveSpecifier(message, index, specifier, what,
		property);
}
