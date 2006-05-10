/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef COMMANDS_H
#define COMMANDS_H


#include "accelerant.h"


struct command {
	uint32	opcode;

	uint32 *Data() { return &opcode; }
	virtual size_t Length() = 0;
};

class QueueCommands {
	public:
		QueueCommands(ring_buffer &ring);
		~QueueCommands();

		void Put(struct command &command);
		void PutFlush();
		void PutWaitFor(uint32 event);
		void PutOverlayFlip(uint32 code, bool updateCoefficients);

	private:
		void _MakeSpace(uint32 size);
		void _Write(uint32 data);

		ring_buffer	&fRingBuffer;
};


// commands


#endif	// COMMANDS_H
