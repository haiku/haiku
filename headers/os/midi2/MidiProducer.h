#ifndef _MIDI_SOURCE_H
#define _MIDI_SOURCE_H

#include <Locker.h>
#include <MidiEndpoint.h>

class BMidiEvent;

class BMidiProducer : public BMidiEndpoint
{
public:
	status_t Connect(BMidiConsumer *toObject);
	status_t Disconnect(BMidiConsumer *toObject);
	bool IsConnected(BMidiConsumer *toObject) const;
	BList* Connections() const;

private:
	friend class BMidiLocalProducer;
	friend class BMidiRoster;
	
	BMidiProducer(const char *name = NULL);	
	virtual ~BMidiProducer();

	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();		
		
	BMidiList *fConnections;
	mutable BLocker fLock;
	
	int fConnectionCount;
	uint32 _reserved[1];
};

class BMidiLocalProducer : public BMidiProducer
{
public:
	BMidiLocalProducer(const char *name = NULL);	
	
	virtual void Connected(BMidiConsumer *dest);
	virtual void Disconnected(BMidiConsumer *dest);
	
	// Spray some MIDI data downstream to the targets of this object
	void SprayData(void *data, size_t length, 
				  bool atomic = false, bigtime_t time = 0) const;
	
	void SprayNoteOff(uchar channel, uchar note, uchar velocity, 
					 bigtime_t time = 0) const;
	void SprayNoteOn(uchar channel, uchar note, uchar velocity, 
					bigtime_t time = 0) const;
	void SprayKeyPressure(uchar channel, uchar note, uchar pressure, 
						 bigtime_t time = 0) const;
	void SprayControlChange(uchar channel, uchar controlNumber, 
						   uchar controlValue, bigtime_t time = 0) const;
	void SprayProgramChange(uchar channel, uchar programNumber, 
						   bigtime_t time = 0) const;
	void SprayChannelPressure(uchar channel, uchar pressure, 
							 bigtime_t time = 0) const;
	void SprayPitchBend(uchar channel, uchar lsb, uchar msb, 
					   bigtime_t time = 0) const;
	void SpraySystemExclusive(void* data, size_t dataLength, 
							 bigtime_t time = 0) const;
	void SpraySystemCommon(uchar statusByte, uchar data1, uchar data2, 
						  bigtime_t time = 0) const;
	void SpraySystemRealTime(uchar statusByte, 
							bigtime_t time = 0) const; 
	void SprayTempoChange(int32 bpm, 
						 bigtime_t time = 0) const;	
	
protected:
	~BMidiLocalProducer();
	
private:
	void SprayEvent(BMidiEvent *event, size_t length) const;
	
	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();

	uint32 _reserved[2];		
};

#endif 
