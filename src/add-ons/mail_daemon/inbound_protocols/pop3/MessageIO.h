#ifndef ZOIDBERG_MAIL_MESSAGE_IO_H
#define ZOIDBERG_MAIL_MESSAGE_IO_H
/* MessageIO - Glue code for reading/writing messages directly from the
** protocols but present a BPositionIO interface to the caller, while caching
** the data read/written in a slave file.
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <DataIO.h>
#include <Path.h>
#include <Message.h>

class SimpleMailProtocol;

class BMailMessageIO : public BPositionIO {
	public:
		BMailMessageIO(SimpleMailProtocol *protocol, BPositionIO *dump_to, int32 seq_id);
		~BMailMessageIO();
		
		//----BPositionIO
		virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t amountToRead);
		virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t amountToWrite);
		
		virtual off_t		Seek(off_t position, uint32 seek_mode);
		virtual	off_t		Position() const;
		
	private:
		void ResetSize(void);
		
		BPositionIO *slave;
		int32 message_id;
		SimpleMailProtocol *network;
		
		size_t size;
		enum MessageIOStateEnum {
			READ_HEADER_NEXT,
			READ_BODY_NEXT,
			ALL_READING_DONE
		} state;
};

#endif	/* ZOIDBERG_MAIL_MESSAGE_IO_H */
