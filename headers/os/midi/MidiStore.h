
#ifndef _MIDI_STORE_H
#define _MIDI_STORE_H

#include <BeBuild.h>
#include <Midi.h>
#include <MidiSynthFile.h>

struct entry_ref;

class BFile;
class BList;
struct BMidiEvent;

class BMidiStore : public BMidi {
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
		void* data, size_t length, uint32 time = B_NOW);

	virtual void SystemCommon(
		uchar status, uchar data1, uchar data2, uint32 time = B_NOW);

	virtual void SystemRealTime(uchar status, uint32 time = B_NOW);

	virtual void TempoChange(int32 beatsPerMinute, uint32 time = B_NOW);

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

	friend class BMidiSynthFile;

	virtual void _ReservedMidiStore1();
	virtual void _ReservedMidiStore2();
	virtual void _ReservedMidiStore3();

	virtual void Run();

	void AddEvent(BMidiEvent* event);
	void SprayEvent(const BMidiEvent* event, uint32 time);
	BMidiEvent* EventAt(int32 index) const;
	uint32 GetEventTime(const BMidiEvent* event) const;
	uint32 TicksToMilliseconds(uint32 ticks) const;
	uint32 MillisecondsToTicks(uint32 ms) const;

	BList* fEvents;
	int32 fCurrentEvent;
	uint32 fStartTime;
	int32 fBeatsPerMinute;
	int16 fTicksPerBeat;
	bool fNeedsSorting;

	void ReadFourCC(char* fourcc);
	uint32 Read32Bit();
	uint16 Read16Bit();
	uint8 PeekByte();
	uint8 NextByte();
	void SkipBytes(uint32 length);
	uint32 ReadVarLength();
	void ReadChunk();
	void ReadTrack();
	void ReadSystemExclusive();
	void ReadMetaEvent();

	void WriteFourCC(char a, char b, char c, char d);
	void Write32Bit(uint32 val);
	void Write16Bit(uint16 val);
	void WriteByte(uint8 val);
	void WriteVarLength(uint32 val);
	void WriteTrack();
	void WriteMetaEvent(BMidiEvent* event);

	BFile* fFile;
	uint32 fByteCount;
	uint32 fTotalTicks;
	uint16 fNumTracks;
	uint16 fCurrTrack;
	uint16 fFormat;

	uint16 _reserved1[1];

	bool* fInstruments;
	synth_file_hook fHookFunc;
	int32 fHookArg;
	bool fLooping;
	bool fPaused;
	bool fFinished;

	uint32 _reserved2[12];
};

#endif // _MIDI_STORE_H
