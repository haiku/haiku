// CountEventConsumer.h
// --------------------
// A simple MIDI consumer that counts incoming MIDI events.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _CountEventConsumer_h
#define _CountEventConsumer_h

#include <MidiConsumer.h>
#include <SupportDefs.h>

class CountEventConsumer : public BMidiLocalConsumer
{
public:
	CountEventConsumer(const char* name)	
		: BMidiLocalConsumer(name), m_eventCount(0)
	{}
	void Reset() { m_eventCount = 0; }
	int32 CountEvents() { return m_eventCount; }
	
	void Data(uchar*, size_t, bool, bigtime_t)
	{ atomic_add(&m_eventCount, 1); }
	
private:
	int32 m_eventCount;
};

#endif /* _CountEventConsumer_h */
