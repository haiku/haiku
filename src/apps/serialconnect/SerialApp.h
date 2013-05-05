/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#ifndef _SERIALAPP_H_
#define _SERIALAPP_H_


#include <Application.h>
#include <SerialPort.h>


class BFile;
class SerialWindow;


class SerialApp: public BApplication
{
	public:
		SerialApp();
		~SerialApp();
		void ReadyToRun();
		void MessageReceived(BMessage* message);

	private:
		BSerialPort fSerialPort;
		sem_id fSerialLock;
		SerialWindow* fWindow;
		BFile* fLogFile;

		static status_t PollSerial(void*);

		static const char* kApplicationSignature;
};


enum messageConstants {
	kMsgDataRead  = 'dare',
	kMsgDataWrite = 'dawr',
	kMsgLogfile   = 'logf',
	kMsgOpenPort  = 'open',
	kMsgSettings  = 'stty',
};

#endif
