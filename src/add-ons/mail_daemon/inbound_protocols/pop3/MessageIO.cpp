/* BMailMessageIO - Glue code for reading/writing messages directly from the
** protocols but present a BPositionIO interface to the caller, while caching
** the data read/written in a slave file.
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <BeBuild.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "MessageIO.h"
#include "SimpleMailProtocol.h"

BMailMessageIO::BMailMessageIO(SimpleMailProtocol *protocol, BPositionIO *dump_to, int32 seq_id) :
	slave(dump_to),
	message_id(seq_id),
	network(protocol),
	size(0),
	state(READ_HEADER_NEXT) {
		//-----do nothing, and do it well-----
	}


ssize_t	BMailMessageIO::ReadAt(off_t pos, void *buffer, size_t amountToRead) {
	status_t errorCode;
	char     lastBytes [5];
	off_t    old_pos = slave->Position();

	while ((pos + amountToRead) > size) {
		if (state >= ALL_READING_DONE)
			break;
		switch (state) {
			// Read (download from the mail server) just the message headers,
			// and append a blank line if needed (so the header processing code
			// can tell where the end of the headers is).  Don't append too
			// much otherwise the part after the header will appear mangled
			// when it is overwritten in a full read.  This can be useful for
			// filters which discard the message after only reading the header,
			// thus avoiding the time it takes to download the whole message.
			case READ_HEADER_NEXT:
				slave->SetSize(0); // Truncate the file.
				slave->Seek(0,SEEK_SET);
				errorCode = network->GetHeader(message_id,slave);
				if (errorCode < 0)
					return errorCode;
				// See if it already ends in a blank line, if not, add enough
				// end-of-lines to give a blank line.
				slave->Seek (-4, SEEK_END);
				strcpy (lastBytes, "xxxx");
				slave->Read (lastBytes, 4);
				if (strcmp (lastBytes, "\r\n\r\n") != 0) {
					if (strcmp (lastBytes + 2, "\r\n") == 0)
						slave->Write("\r\n", 2);
					else
						slave->Write("\r\n\r\n", 4);
				}
				state = READ_BODY_NEXT;
				break;

			// OK, they want more than the headers.  Read the whole message,
			// starting from the beginning (network->Retrieve does a seek to
			// the start of the file for POP3 so we have to read the whole
			// thing).  This wastes a slight amount of time on high speed
			// connections, and on dial-up modem connections, hopefully the
			// modem's V.90 data compression will make it very quick to
			// retransmit the header portion.
			case READ_BODY_NEXT:
				slave->SetSize(0); // Truncate the file.
				slave->Seek(0,SEEK_SET);
				errorCode = network->Retrieve(message_id,slave);
				if (errorCode < 0)
					return errorCode;
				state = ALL_READING_DONE;
				break;

			default: // Shouldn't happen.
				return -1;
		}
		ResetSize();
	}

	// Put the file position back at where it was, if possible.  That's because
	// ReadAt isn't supposed to affect the file position.
	if (old_pos < size)
		slave->Seek (old_pos, SEEK_SET);
	else
		slave->Seek (0, SEEK_END);

	return slave->ReadAt(pos,buffer,amountToRead);
}


ssize_t	BMailMessageIO::WriteAt(off_t pos, const void *buffer, size_t amountToWrite) {
	ssize_t return_val;

	return_val = slave->WriteAt(pos,buffer,amountToWrite);
	ResetSize();

	return return_val;
}

off_t BMailMessageIO::Seek(off_t position, uint32 seek_mode) {
	ssize_t errorCode;
	char    tempBuffer [1];
	
	if (seek_mode == SEEK_END) {
		if (state != ALL_READING_DONE) {
			// Force it to read the whole message to find the size of it.
			state = READ_BODY_NEXT; // Skip the header reading step.
			errorCode = ReadAt (size + 1, tempBuffer, sizeof (tempBuffer));
			if (errorCode < 0)
				return errorCode;
		}
	}
	return slave->Seek(position,seek_mode);
}

off_t BMailMessageIO::Position() const {
	return slave->Position();
}

void BMailMessageIO::ResetSize(void) {
	off_t old = slave->Position();

	slave->Seek(0,SEEK_END);
	size = slave->Position();

	slave->Seek(old,SEEK_SET);
}

BMailMessageIO::~BMailMessageIO() {
	delete slave;
}

