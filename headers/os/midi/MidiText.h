//-----------------------------------------------------------------------------
#ifndef _MIDI_TEXT_H_
#define _MIDI_TEXT_H_

#include <Midi.h>

class BMidiText : public BMidi {
public:
	BMidiText();
	virtual ~BMidiText();

	virtual void NoteOff(uchar channel, uchar note,
		uchar velocity, uint32 time = B_NOW);
	virtual void NoteOn(uchar channel, uchar note,
		uchar velocity, uint32 time = B_NOW);
	virtual void KeyPressure(uchar channel, uchar note,
		uchar pressure, uint32 time = B_NOW);
	virtual void ControlChange(uchar channel, uchar control_number,
		uchar control_value, uint32 time = B_NOW);
	virtual void ProgramChange(uchar channel, uchar program_number,
		uint32 time = B_NOW);
	virtual void ChannelPressure(uchar channel, uchar pressure,
		uint32 time = B_NOW);
	virtual void PitchBend(uchar channel, uchar lsb,
		uchar msb, uint32 time = B_NOW);
	virtual void SystemExclusive(void * data, size_t data_length,
		uint32 time = B_NOW);
	virtual void SystemCommon(uchar status_byte, uchar data1,
		uchar data2, uint32 time = B_NOW);
	virtual void SystemRealTime(uchar status_byte, uint32 time = B_NOW);
	virtual void TempoChange(int32 beats_per_minute, uint32 time = B_NOW);
	virtual void AllNotesOff(bool just_channel = true, uint32 time = B_NOW);

	void ResetTimer(bool start = false);

private:
	int32 _start_time;

private:
	void _PrintTime();
};

#endif _MIDI_TEXT_H_
