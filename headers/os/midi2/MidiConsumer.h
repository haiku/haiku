#ifndef _MIDI_SINK_H
#define _MIDI_SINK_H

#include <MidiEndpoint.h>

class BMidiDispatcher;

class BMidiConsumer : public BMidiEndpoint
{
public:
	bigtime_t Latency() const;
	
private:
	friend class BMidiLocalConsumer;
	friend class BMidiLocalProducer;
	friend class BMidiRoster;
	
	BMidiConsumer(const char *name = NULL);	
	virtual ~BMidiConsumer();
	
	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();
	
	port_id fEventPort;
	bigtime_t fLatency;
	
	uint32 _reserved[2];
};

class BMidiLocalConsumer : public BMidiConsumer
{
public:
	BMidiLocalConsumer(const char *name = NULL);
	
	void SetLatency(bigtime_t latency);
	int32 GetProducerID(void);
	
	void SetTimeout(bigtime_t when, void *data);	
	virtual void Timeout(void *data);
	
	virtual void Data(uchar *data, size_t length, bool atomic,
					  bigtime_t time);	
	// Raw Midi data has been received.
	// The default implementation calls one of the virtuals below:
	
	virtual	void NoteOff(uchar channel, uchar note, uchar velocity, 
						 bigtime_t time);
	virtual	void NoteOn(uchar channel, uchar note, uchar velocity, 
						bigtime_t time);
	virtual	void KeyPressure(uchar channel, uchar note, uchar pressure, 
							 bigtime_t time);
	virtual	void ControlChange(uchar channel, uchar controlNumber, 
							   uchar controlValue, 
							   bigtime_t time);
	virtual	void ProgramChange(uchar channel, uchar programNumber, 
							   bigtime_t time);
	virtual	void ChannelPressure(uchar channel, uchar pressure, 
								 bigtime_t time);
	virtual	void PitchBend(uchar channel, uchar lsb, uchar msb, 
						   bigtime_t time);
	virtual	void SystemExclusive(void* data, size_t dataLength, 
								 bigtime_t time);
	virtual	void SystemCommon(uchar statusByte, uchar data1, uchar data2, 
							  bigtime_t time);
	virtual	void SystemRealTime(uchar statusByte, 
								bigtime_t time);
	virtual	void TempoChange(int32 bpm, 
							 bigtime_t time);
	virtual void AllNotesOff(bool justChannel, 
							 bigtime_t time);
protected:	
	~BMidiLocalConsumer();
 
private:

	friend class BMidiRoster;
	friend class BMidiDispatcher;
	
	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();
		
	BMidiDispatcher *fDispatcher;
	bigtime_t fTimeout;
	void *fTimeoutData;
	
	int32 fCurrentProducer;
	uint32 _reserved[1];
};

#endif
