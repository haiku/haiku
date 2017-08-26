/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "FileSender.h"

#include "SerialApp.h"

#include <DataIO.h>
#include <Message.h>
#include <SerialPort.h>


FileSender::~FileSender()
{
}


RawSender::RawSender(BDataIO* source, BSerialPort* sink, BHandler* listener)
{
	// FIXME doing this all here in the constructor is not good. We need to
	// do things asynchronously instead so as not to lock the application
	// thread.
	off_t sourceSize;
	off_t position;

	BPositionIO* pos = dynamic_cast<BPositionIO*>(source);
	if (pos)
		pos->GetSize(&sourceSize);
	else
		sourceSize = 0;
	position = 0;

	BMessenger messenger(listener);

	uint8_t buffer[256];
	for (;;) {
		ssize_t s = source->Read(&buffer, sizeof(buffer));
		if (s <= 0)
			return;

		sink->Write(buffer, s);
		position += s;

		BMessage msg(kMsgProgress);
		msg.AddInt32("pos", position);
		msg.AddInt32("size", sourceSize);
		msg.AddString("info", "Sending" B_UTF8_ELLIPSIS);
		messenger.SendMessage(&msg);

		//usleep(20000);
	}
}


RawSender::~RawSender()
{
}


bool
RawSender::BytesReceived(const uint8_t* data, size_t length)
{
	// Nothing to do with received bytes, this protocol has no kind of
	// acknowledgement from remote side.
	return true;
}
