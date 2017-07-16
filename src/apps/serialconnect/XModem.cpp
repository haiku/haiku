/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "XModem.h"

#include "SerialApp.h"

#include <String.h>

#include <stdio.h>
#include <string.h>


// ASCII control characters used in XMODEM protocol
static const char kSOH =  1;
static const char kEOT =  4;
static const char kACK =  6;
static const char kNAK = 21;
static const char kSUB = 26;

static const int kBlockSize = 128;


XModemSender::XModemSender(BDataIO* source, BSerialPort* sink, BHandler* listener)
	: fSource(source),
	fSink(sink),
	fListener(listener),
	fBlockNumber(0),
	fEotSent(false)
{
	fStatus = "Waiting for receiver" B_UTF8_ELLIPSIS;

	BPositionIO* pos = dynamic_cast<BPositionIO*>(source);
	if (pos)
		pos->GetSize(&fSourceSize);
	else
		fSourceSize = 0;

	NextBlock();
}


XModemSender::~XModemSender()
{
	delete fSource;
}


bool
XModemSender::BytesReceived(const char* data, size_t length)
{
	size_t i;

	for (i = 0; i < length; i++)
	{
		switch (data[i])
		{
			case kNAK:
				if (fEotSent) {
					fSink->Write(&kEOT, 1);
				} else {
					fStatus = "Checksum error, re-send block";
					SendBlock();
				}
				break;

			case kACK:
				if (fEotSent) {
					return true;
				}

				if (NextBlock() == B_OK) {
					fStatus = "Sending" B_UTF8_ELLIPSIS;
					SendBlock();
				} else {
					fStatus = "Everything sent, waiting for acknowledge";
					fSink->Write(&kEOT, 1);
					fEotSent = true;
				}
				break;

			default:
				break;
		}
	}

	return false;
}


void
XModemSender::SendBlock()
{
	uint8_t header[3];
	uint8_t checksum = 0;
	int i;

	header[0] = kSOH;
	header[1] = fBlockNumber;
	header[2] = 255 - fBlockNumber;

	for (i = 0; i < kBlockSize; i++)
		checksum += fBuffer[i];

	fSink->Write(header, 3);
	fSink->Write(fBuffer, kBlockSize);
	fSink->Write(&checksum, 1);
}


status_t
XModemSender::NextBlock()
{
	memset(fBuffer, kSUB, kBlockSize);

	if (fSource->Read(fBuffer, kBlockSize) > 0) {
		fBlockNumber++;
		BMessage msg(kMsgProgress);
		msg.AddInt32("pos", fBlockNumber);
		msg.AddInt32("size", fSourceSize / kBlockSize);
		msg.AddString("info", fStatus);
		fListener.SendMessage(&msg);
		return B_OK;
	}
	return B_ERROR;
}
