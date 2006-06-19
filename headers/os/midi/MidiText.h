
#ifndef _MIDI_TEXT_H
#define _MIDI_TEXT_H

#include <BeBuild.h>
#include <Midi.h>
#include <stdio.h>

class BMidiText : public BMidi 
{
public:

	BMidiText();
	virtual ~BMidiText();

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

	void ResetTimer(bool start = false);

private:

	virtual void _ReservedMidiText1();
	virtual void _ReservedMidiText2();
	virtual void _ReservedMidiText3();

	virtual void Run();

	void _WaitAndPrint(uint32 time);

	uint32 fStartTime;
	uint32 _reserved[4];
};

#endif // _MIDI_TEXT_H

