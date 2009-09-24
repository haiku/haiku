/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jerome Leveque
 *		Philippe Houdoin
 */
#ifndef PORT_DRIVERS_H
#define PORT_DRIVERS_H


#include <MidiProducer.h>
#include <MidiConsumer.h>

class MidiPortConsumer : public BMidiLocalConsumer {
public:
					MidiPortConsumer(int fd, const char* path);

	void 			Data(uchar* data, size_t length, bool atomic, bigtime_t time);

private:
	int fFileDescriptor;
};


class MidiPortProducer : public BMidiLocalProducer {
public:
					MidiPortProducer(int fd, const char* path = NULL);
	virtual			~MidiPortProducer(void);

	int32 			GetData(void);

private:
	static int32 	_ReaderThread(void* data);

	int 			fFileDescriptor;
	volatile bool 	fKeepRunning;
	thread_id		fReaderThread;
};

#endif // PORT_DRIVERS_H
