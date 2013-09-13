/* CountEventConsumer.h
 * --------------------
 * A simple MIDI consumer that counts incoming MIDI events.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef COUNTEVENTCONSUMER_H
#define COUNTEVENTCONSUMER_H

#include <MidiConsumer.h>
#include <SupportDefs.h>

class CountEventConsumer : public BMidiLocalConsumer
{
public:
	CountEventConsumer(const char* name)	
		:
		BMidiLocalConsumer(name),
		fEventCount(0)
	{}
	void Reset()
	{
		fEventCount = 0;
	}
	int32 CountEvents()
	{
		return fEventCount;
	}
	
	void Data(uchar*, size_t, bool, bigtime_t)
	{
		atomic_add(&fEventCount, 1);
	}
	
private:
	int32 fEventCount;
};

#endif /* COUNTEVENTCONSUMER_H */
