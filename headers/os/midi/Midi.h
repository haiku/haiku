
#ifndef _MIDI_H
#define _MIDI_H

#include <BeBuild.h>
#include <MidiDefs.h>
#include <OS.h>

class BMidiEvent;
class BList;

class BMidi 
{
public:
	BMidi();
	virtual ~BMidi();

	virtual void NoteOff(
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
		void* data, size_t dataLength, uint32 time = B_NOW); 

	virtual void SystemCommon(
		uchar status, uchar data1, uchar data2, uint32 time = B_NOW);

	virtual void SystemRealTime(uchar status, uint32 time = B_NOW);

	virtual void TempoChange(int32 beatsPerMinute, uint32 time = B_NOW);

 	virtual void AllNotesOff(bool justChannel = true, uint32 time = B_NOW);

	virtual status_t Start();
	virtual void Stop();
	bool IsRunning() const;
	
	void Connect(BMidi* toObject);
	void Disconnect(BMidi* fromObject);
	bool IsConnected(BMidi* toObject) const;
	BList* Connections() const;

	void SnoozeUntil(uint32 time) const;

protected:

	void SprayNoteOff(
		uchar channel, uchar note, uchar velocity, uint32 time) const;

	void SprayNoteOn(
		uchar channel, uchar note, uchar velocity, uint32 time) const;

	void SprayKeyPressure(
		uchar channel, uchar note, uchar pressure, uint32 time) const;

	void SprayControlChange(
		uchar channel, uchar controlNumber, uchar controlValue, 
		uint32 time) const;

	void SprayProgramChange(
		uchar channel, uchar programNumber, uint32 time) const; 

	void SprayChannelPressure(
		uchar channel, uchar pressure, uint32 time) const;

	void SprayPitchBend(
		uchar channel, uchar lsb, uchar msb, uint32 time) const;

	void SpraySystemExclusive(
		void* data, size_t dataLength, uint32 time = B_NOW) const; 

	void SpraySystemCommon(
		uchar status, uchar data1, uchar data2, uint32 time) const;

	void SpraySystemRealTime(uchar status, uint32 time) const;

	void SprayTempoChange(int32 beatsPerMinute, uint32 time) const;

	bool KeepRunning();

private:

	friend class BMidiStore;

	virtual void _ReservedMidi1();
	virtual void _ReservedMidi2();
	virtual void _ReservedMidi3();

	static int32 RunThread(void* data);
	static int32 InflowThread(void* data);

	virtual void Run();
	void Inflow();

	void SprayMidiEvent(BMidiEvent* event) const;

	BList* connections;
	thread_id runThread;
	thread_id inflowThread;
	port_id inflowPort;
	bool isRunning;
	bool inflowAlive;

	uint32 _reserved[5];
};

#endif // _MIDI_H

