/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 */
#ifndef MAIL_MESSAGE_IO_H
#define MAIL_MESSAGE_IO_H


#include <DataIO.h>
#include <Message.h>
#include <Path.h>

#include "pop3.h"


class BMailMessageIO : public BPositionIO {
public:
								BMailMessageIO(POP3Protocol* protocol,
									BPositionIO* dumpTo, int32 messageID);
								~BMailMessageIO();

		//----BPositionIO
	virtual	ssize_t				ReadAt(off_t pos, void* buffer,
									size_t amountToRead);
	virtual	ssize_t				WriteAt(off_t pos, const void* buffer,
									size_t amountToWrite);

	virtual off_t				Seek(off_t position, uint32 seekMode);
	virtual	off_t				Position() const;

private:
			void				_ResetSize();

private:
			enum MessageIOStateEnum {
				READ_HEADER_NEXT,
				READ_BODY_NEXT,
				ALL_READING_DONE
			};

			BPositionIO*		fSlave;
			int32				fMessageID;
			POP3Protocol*		fProtocol;
			size_t				fSize;
			MessageIOStateEnum	fState;
};


#endif	/* MAIL_MESSAGE_IO_H */
