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
		BSerialPort fSerialPort;
		sem_id fSerialLock;
		SerialWindow* fWindow;

		static status_t PollSerial(void*);

		static const char* kApplicationSignature;
};


enum messageConstants {
	kMsgOpenPort  = 'open',
	kMsgDataRead  = 'dare',
	kMsgDataWrite = 'dawr',
};

