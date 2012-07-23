/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <Application.h>
#include <SerialPort.h>


class SerialWindow;


class SerialApp: public BApplication
{
	public:
		SerialApp();
		void ReadyToRun();
		void MessageReceived(BMessage* message);

	private:
		BSerialPort serialPort;
		static status_t PollSerial(void*);

		sem_id serialLock;
		SerialWindow* fWindow;

		static const char* kApplicationSignature;
};

enum messageConstants {
	kMsgOpenPort = 'open',
	kMsgDataRead = 'dare',
};

