
#ifndef _MIDI_STORE_H
#define _MIDI_STORE_H

#include <BeBuild.h>
#include <Midi.h>

struct entry_ref;

class BMidiEvent;
class BList;
class BFile;

class BMidiStore : public BMidi 
{
public:
	BMidiStore();
	virtual ~BMidiStore();

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

	status_t Import(const entry_ref* ref);
	status_t Export(const entry_ref* ref, int32 format);

	void SortEvents(bool force = false);
	uint32 CountEvents() const;

	uint32 CurrentEvent() const;
	void SetCurrentEvent(uint32 eventNumber);

	uint32 DeltaOfEvent(uint32 eventNumber) const;
	uint32 EventAtDelta(uint32 time) const;

	uint32 BeginTime() const;

	void SetTempo(int32 beatsPerMinute);
	int32 Tempo() const;

private:

	virtual void _ReservedMidiStore1();
	virtual void _ReservedMidiStore2();

	virtual void Run();

	BMidiEvent* EventAt(uint32 index) const;

	static int CompareEvents(const void* event1, const void* event2);

	void ReadFileHeader();
	void ReadTrackHeader();
	void ReadTrack();
	int32 Read32Bit();
	int16 Read16Bit();
	void ReadData(uint8* data, size_t size);

	BList* events;
	uint32 tempo;
	uint32 curEvent;
	uint32 startTime;
	uint32 ticksPerBeat;

	BFile* file;
	int32 numTracks;
	int32 trackSize;
	int16 division;

	uint16 _reserved1;
	uint32 _reserved2[17];
};

#endif // _MIDI_STORE_H

