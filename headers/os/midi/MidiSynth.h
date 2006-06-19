
#ifndef _MIDI_SYNTH_H
#define _MIDI_SYNTH_H

#include <BeBuild.h>
#include <Midi.h>
#include <Synth.h>
#include <MidiDefs.h>

class BSynth;

class BMidiSynth : public BMidi
{
public:

	BMidiSynth(); 
	virtual ~BMidiSynth();

	status_t EnableInput(bool enable, bool loadInstruments);
	bool IsInputEnabled(void) const;

	void SetVolume(double volume);
	double Volume(void) const;

	void SetTransposition(int16 offset);
	int16 Transposition(void) const;

	void MuteChannel(int16 channel, bool do_mute);
	void GetMuteMap(char* pChannels) const;

	void SoloChannel(int16 channel, bool do_solo);
	void GetSoloMap(char* pChannels) const;

	status_t LoadInstrument(int16 instrument);
	status_t UnloadInstrument(int16 instrument);
	status_t RemapInstrument(int16 from, int16 to);

	void FlushInstrumentCache(bool startStopCache);

	uint32 Tick(void) const;

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

	virtual void AllNotesOff(bool justChannel, uint32 time = B_NOW);

protected:

	void* fSongVariables;
	void* fPerformanceVariables;
	bool fMidiQueue;
	
private:

	friend class BSynth;

	virtual void _ReservedMidiSynth1();
	virtual void _ReservedMidiSynth2();
	virtual void _ReservedMidiSynth3();
	virtual void _ReservedMidiSynth4();

	virtual void Run();

	bigtime_t fCreationTime;
	int16 fTranspose;
	bool fInputEnabled;
};

#endif // _MIDI_SYNTH_H
