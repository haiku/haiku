#ifndef _PORT_DRIVER_H
#define _PORT_DRIVER_H

#include <MidiProducer.h>
#include <MidiConsumer.h>

//----------------------------------------------------------

class MidiPortConsumer : public BMidiLocalConsumer
{

public:
	MidiPortConsumer(int filedescriptor, const char *name);

	void Data(uchar *data, size_t length, bool atomic, bigtime_t time);

private:
	int fd;
	
};

//----------------------------------------------------------

class MidiPortProducer : public BMidiLocalProducer
{
public:
	MidiPortProducer(int filedescriptor, const char *name);
	~MidiPortProducer(void);

	static int32 SpawnThread(MidiPortProducer *producer);
	int32 GetData(void);

private:
	int fd;
	bool KeepRunning;
};

//----------------------------------------------------------

#endif
