
#ifndef _MIDI_CONSUMER_H
#define _MIDI_CONSUMER_H

#include <MidiEndpoint.h>

namespace BPrivate { class BMidiRosterLooper; }

class BMidiConsumer : public BMidiEndpoint
{
public:

	bigtime_t Latency() const;
	
private:

	friend class BMidiLocalConsumer;
	friend class BMidiLocalProducer;
	friend class BMidiRoster;
	friend class BPrivate::BMidiRosterLooper;
	
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
	
	port_id fPort;
	bigtime_t fLatency;
	
	uint32 _reserved[2];
};

class BMidiLocalConsumer : public BMidiConsumer
{
public:

	BMidiLocalConsumer(const char *name = NULL);
	
	void SetLatency(bigtime_t latency);
	int32 GetProducerID();
	
	void SetTimeout(bigtime_t when, void *data);	
	virtual void Timeout(void *data);
	
	virtual void Data(
		uchar *data, size_t length, bool atomic, bigtime_t time);	
	
	virtual	void NoteOff(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual	void NoteOn(
		uchar channel, uchar note, uchar velocity, bigtime_t time);

	virtual	void KeyPressure(
		uchar channel, uchar note, uchar pressure, bigtime_t time);

	virtual	void ControlChange(
		uchar channel, uchar controlNumber, uchar controlValue, 
		bigtime_t time);

	virtual	void ProgramChange(
		uchar channel, uchar programNumber, bigtime_t time);

	virtual	void ChannelPressure(
		uchar channel, uchar pressure, bigtime_t time);

	virtual	void PitchBend(
		uchar channel, uchar lsb, uchar msb, bigtime_t time);

	virtual	void SystemExclusive(
		void* data, size_t length, bigtime_t time);

	virtual	void SystemCommon(
		uchar status, uchar data1, uchar data2, bigtime_t time);

	virtual	void SystemRealTime(uchar status, bigtime_t time);

	virtual	void TempoChange(int32 beatsPerMinute, bigtime_t time);

	virtual void AllNotesOff(bool justChannel, bigtime_t time);

protected:	

	~BMidiLocalConsumer();
 
private:

	friend class BMidiRoster;
	friend int32 _midi_event_thread(void *);
	
	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();

	int32 EventThread();

	bigtime_t fTimeout;
	void* fTimeoutData;
	int32 fCurrentProducer;
	thread_id fThread;

	uint32 _reserved[1];
};

#endif // _MIDI_CONSUMER_H
