/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 */


/*!
	Glue code for reading/writing messages directly from the protocols but
	present a BPositionIO interface to the caller, while caching the data
	read/written in a slave file.
*/


#include "MessageIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


BMailMessageIO::BMailMessageIO(POP3Protocol* protocol, BPositionIO* dumpTo,
	int32 messageID)
	:
	fSlave(dumpTo),
	fMessageID(messageID),
	fProtocol(protocol),
	fSize(0),
	fState(READ_HEADER_NEXT)
{
}


BMailMessageIO::~BMailMessageIO()
{
}


ssize_t
BMailMessageIO::ReadAt(off_t pos, void* buffer, size_t amountToRead)
{
	char lastBytes[5];
	off_t oldPosition = fSlave->Position();

	while (pos + amountToRead > fSize) {
		if (fState >= ALL_READING_DONE)
			break;

		switch (fState) {
			// Read (download from the mail server) just the message headers,
			// and append a blank line if needed (so the header processing code
			// can tell where the end of the headers is).  Don't append too
			// much otherwise the part after the header will appear mangled
			// when it is overwritten in a full read.  This can be useful for
			// filters which discard the message after only reading the header,
			// thus avoiding the time it takes to download the whole message.
			case READ_HEADER_NEXT:
			{
				fSlave->SetSize(0); // Truncate the file.
				fSlave->Seek(0,SEEK_SET);
				status_t status = fProtocol->GetHeader(fMessageID, fSlave);
				if (status != B_OK)
					return status;
				// See if it already ends in a blank line, if not, add enough
				// end-of-lines to give a blank line.
				fSlave->Seek(-4, SEEK_END);
				strcpy(lastBytes, "xxxx");
				fSlave->Read(lastBytes, 4);

				if (strcmp(lastBytes, "\r\n\r\n") != 0) {
					if (strcmp(lastBytes + 2, "\r\n") == 0)
						fSlave->Write("\r\n", 2);
					else
						fSlave->Write("\r\n\r\n", 4);
				}
				fState = READ_BODY_NEXT;
				break;
			}

			// OK, they want more than the headers.  Read the whole message,
			// starting from the beginning (network->Retrieve does a seek to
			// the start of the file for POP3 so we have to read the whole
			// thing).  This wastes a slight amount of time on high speed
			// connections, and on dial-up modem connections, hopefully the
			// modem's V.90 data compression will make it very quick to
			// retransmit the header portion.
			case READ_BODY_NEXT:
			{
				fSlave->SetSize(0); // Truncate the file.
				fSlave->Seek(0,SEEK_SET);
				status_t status = fProtocol->Retrieve(fMessageID, fSlave);
				if (status < 0)
					return status;
				fState = ALL_READING_DONE;
				break;
			}

			default: // Shouldn't happen.
				return -1;
		}
		_ResetSize();
	}

	// Put the file position back at where it was, if possible.  That's because
	// ReadAt isn't supposed to affect the file position.
	if (oldPosition < fSize)
		fSlave->Seek (oldPosition, SEEK_SET);
	else
		fSlave->Seek (0, SEEK_END);

	return fSlave->ReadAt(pos, buffer, amountToRead);
}


ssize_t
BMailMessageIO::WriteAt(off_t pos, const void* buffer, size_t amountToWrite)
{
	ssize_t bytesWritten = fSlave->WriteAt(pos, buffer, amountToWrite);
	_ResetSize();

	return bytesWritten;
}


off_t
BMailMessageIO::Seek(off_t position, uint32 seekMode)
{
	if (seekMode == SEEK_END && fState != ALL_READING_DONE) {
		// Force it to read the whole message to find the size of it.
		char tempBuffer;
		fState = READ_BODY_NEXT;
			// Skip the header reading step.
		ssize_t bytesRead = ReadAt(fSize + 1, &tempBuffer, sizeof(tempBuffer));
		if (bytesRead < 0)
			return bytesRead;
	}
	return fSlave->Seek(position, seekMode);
}


off_t
BMailMessageIO::Position() const
{
	return fSlave->Position();
}


void
BMailMessageIO::_ResetSize()
{
	off_t old = fSlave->Position();

	fSlave->Seek(0,SEEK_END);
	fSize = fSlave->Position();

	fSlave->Seek(old,SEEK_SET);
}
