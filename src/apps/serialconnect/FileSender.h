/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#ifndef FILESENDER_H
#define FILESENDER_H


#include <stdint.h>
#include <string.h>


class BDataIO;
class BHandler;
class BSerialPort;


class FileSender {
	public:
		virtual				~FileSender();
		virtual	bool		BytesReceived(const uint8_t* data,
								size_t length) = 0;
};


class RawSender: public FileSender {
	public:
								RawSender(BDataIO* source, BSerialPort* sink,
									BHandler* listener);
		virtual					~RawSender();

		virtual	bool			BytesReceived(const uint8_t* data,
									size_t length);

};


#endif /* !FILESENDER_H */
