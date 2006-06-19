
#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include <Midi.h>

class BMidiConsumer;
class BMidiProducer;

namespace BPrivate { class BMidiPortGlue; }

class BMidiPort : public BMidi 
{
public:

	BMidiPort(const char* name = NULL);
	~BMidiPort();

	status_t InitCheck() const; 
	status_t Open(const char* name);
	void Close();

	const char* PortName() const;

	virtual	void NoteOff(
		uchar channel, uchar note, uchar velocity, uint32 time = B_NOW);

	virtual void NoteOn(
		uchar channel, uchar note, uchar velocity, uint32 time = B_NOW);

	virtual void KeyPressure(
		uchar channel, uchar note, uchar pressure, uint32 time = B_NOW);

	virtual void ControlChange(
		uchar channel, uchar controlNumber, uchar controlValue, 
		uint32 time = B_NOW); 

	virtual void ProgramChange(
		uchar channel, uchar programNumber, uint32 time = B_NOW);

	virtual void ChannelPressure(
		uchar channel, uchar pressure, uint32 time = B_NOW);

	virtual void PitchBend(
		uchar channel, uchar lsb, uchar msb, uint32 time = B_NOW);

	virtual void SystemExclusive(
		void* data, size_t length, uint32 time = B_NOW);

	virtual void SystemCommon(
		uchar status, uchar data0, uchar data2, uint32 time = B_NOW);

	virtual void SystemRealTime(uchar status, uint32 time = B_NOW);

	virtual status_t Start();
	virtual void Stop();

	int32 CountDevices();

	status_t GetDeviceName(
		int32 n, char* name, size_t bufSize = B_OS_NAME_LENGTH);

private:

	typedef BMidi super;

	friend class BPrivate::BMidiPortGlue;

	virtual void _ReservedMidiPort1();
	virtual void _ReservedMidiPort2();
	virtual void _ReservedMidiPort3();

	virtual void Run();

	void ScanDevices();
	void EmptyDeviceList();

	BMidiLocalProducer* fLocalSource;
	BMidiLocalConsumer* fLocalSink;
	BMidiProducer* fRemoteSource;
	BMidiConsumer* fRemoteSink;
		
	char* fPortName;
	status_t fStatus;
	BList* fDevices;

	uint32 _reserved[1];
};

#endif // _MIDI_PORT_H
