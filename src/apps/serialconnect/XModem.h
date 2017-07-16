/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#ifndef XMODEM_H
#define XMODEM_H


#include <Messenger.h>
#include <String.h>


class BDataIO;
class BHandler;
class BSerialPort;


class XModemSender {
	public:
						XModemSender(BDataIO* source, BSerialPort* sink,
							BHandler* listener);
						~XModemSender();

		bool			BytesReceived(const char* data, size_t length);
	private:

		void			SendBlock();
		status_t		NextBlock();

	private:
		BDataIO*		fSource;
		BSerialPort*	fSink;
		BMessenger		fListener;
		off_t			fBlockNumber;
		off_t			fSourceSize;
		uint8_t			fBuffer[128];
		bool			fEotSent;
		BString			fStatus;
};


#endif /* !XMODEM_H */
