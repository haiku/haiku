/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "XModem.h"

#include "SerialApp.h"

#include <Catalog.h>
#include <String.h>

#include <stdio.h>
#include <string.h>


#define B_TRANSLATION_CONTEXT "XModemStatus"


// ASCII control characters used in XMODEM protocol
static const char kSOH =  1;
static const char kEOT =  4;
static const char kACK =  6;
static const char kNAK = 21;
static const char kCAN = 24;
static const char kSUB = 26;

static const int kBlockSize = 128;


XModemSender::XModemSender(BDataIO* source, BSerialPort* sink, BHandler* listener)
	: fSource(source),
	fSink(sink),
	fListener(listener),
	fBlockNumber(0),
	fEotSent(false),
	fUseCRC(false)
{
	fStatus = B_TRANSLATE("Waiting for receiver" B_UTF8_ELLIPSIS);

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
XModemSender::BytesReceived(const uint8_t* data, size_t length)
{
	size_t i;

	for (i = 0; i < length; i++)
	{
		switch (data[i])
		{
			case 'C':
				// A 'C' to request the first block is a request to use a CRC
				// in place of an 8-bit checksum.
				// In any other place, it is ignored.
				if (fBlockNumber <= 1) {
					fStatus = B_TRANSLATE("CRC requested");
					fUseCRC = true;
					SendBlock();
				} else
					break;
			case kNAK:
				if (fEotSent) {
					fSink->Write(&kEOT, 1);
				} else {
					fStatus = B_TRANSLATE("Checksum error, re-send block");
					SendBlock();
				}
				break;

			case kACK:
				if (fEotSent) {
					return true;
				}

				if (NextBlock() == B_OK) {
					fStatus = B_TRANSLATE("Sending" B_UTF8_ELLIPSIS);
					SendBlock();
				} else {
					fStatus = B_TRANSLATE("Everything sent, "
						"waiting for acknowledge");
					fSink->Write(&kEOT, 1);
					fEotSent = true;
				}
				break;

			case kCAN:
			{
				BMessage msg(kMsgProgress);
				msg.AddInt32("pos", 0);
				msg.AddInt32("size", 0);
				msg.AddString("info",
					B_TRANSLATE("Remote cancelled transfer"));
				fListener.SendMessage(&msg);
				return true;
			}

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

	fSink->Write(header, 3);
	fSink->Write(fBuffer, kBlockSize);

	if (fUseCRC) {
		uint16_t crc = CRC(fBuffer, kBlockSize);
		uint8_t crcBuf[2];
		crcBuf[0] = crc >> 8;
		crcBuf[1] = crc & 0xFF;
		fSink->Write(crcBuf, 2);
	} else {
		// Use a traditional (and fragile) checksum
		for (i = 0; i < kBlockSize; i++)
			checksum += fBuffer[i];

		fSink->Write(&checksum, 1);
	}
}


status_t
XModemSender::NextBlock()
{
	memset(fBuffer, kSUB, kBlockSize);

	if (fSource->Read(fBuffer, kBlockSize) > 0) {
		// Notify for progress bar update
		BMessage msg(kMsgProgress);
		msg.AddInt32("pos", fBlockNumber);
		msg.AddInt32("size", fSourceSize / kBlockSize);
		msg.AddString("info", fStatus);
		fListener.SendMessage(&msg);

		// Remember that we moved to next block
		fBlockNumber++;
		return B_OK;
	}
	return B_ERROR;
}

uint16_t XModemSender::CRC(const uint8_t *buf, size_t len)
{
	uint16_t crc = 0;
	while( len-- ) {
		int i;
		crc ^= ((uint16_t)(*buf++)) << 8;
		for( i = 0; i < 8; ++i ) {
			if( crc & 0x8000 )
				crc = (crc << 1) ^ 0x1021;
			else
				crc = crc << 1;
		}
	}
	return crc;
}
