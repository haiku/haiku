/*******************************************************************************
/
/	File:		MidiSynth.h
/
/	Description:	Midi object that talks to the General MIDI synthesizer.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MIDI_SYNTH_H
#define _MIDI_SYNTH_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <Midi.h>
#include <Synth.h>
#include <MidiDefs.h>


class BMidiSynth : public BMidi
{
public:
				BMidiSynth(); 
	virtual		~BMidiSynth();


	status_t	EnableInput(bool enable, bool loadInstruments);
	bool 		IsInputEnabled(void) const;
		
	/* set volume. You can overdrive by passing values larger than 1.0*/
	void		SetVolume(double volume);
	double		Volume(void) const;

	/* set note offset in semi tones	*/
	/* (12 is down an octave, -12 is up an octave)*/
	void 		SetTransposition(int16 offset);
	int16		Transposition(void) const;

	/* Mute and unmute channels (0 to 15)*/
	void		MuteChannel(int16 channel, bool do_mute);
	void		GetMuteMap(char *pChannels) const;

	void		SoloChannel(int16 channel, bool do_solo);
	void		GetSoloMap(char *pChannels) const;

	/* Use these functions to drive the synth engine directly*/
	status_t		LoadInstrument(int16 instrument);
	status_t		UnloadInstrument(int16 instrument);
	status_t		RemapInstrument(int16 from, int16 to);

	void		FlushInstrumentCache(bool startStopCache);

	/* get current midi tick in microseconds*/
	uint32	Tick(void) const;

	/* The channel variable is 1 to 16. Channel 10 is percussion for example.*/
	/* The programNumber variable is a number from 0-127*/

virtual	void	NoteOff(uchar channel, 
						uchar note, 
						uchar velocity,
						uint32 time = B_NOW);

virtual	void	NoteOn(uchar channel, 
					   uchar note, 
					   uchar velocity,
			    	   uint32 time = B_NOW);

virtual	void	KeyPressure(uchar channel, 
							uchar note, 
							uchar pressure,
							uint32 time = B_NOW);

virtual	void	ControlChange(uchar channel, 
							  uchar controlNumber,
							  uchar controlValue, 
							  uint32 time = B_NOW);

virtual	void	ProgramChange(uchar channel, 
								uchar programNumber,
							  	uint32 time = B_NOW);

virtual	void	ChannelPressure(uchar channel, 
								uchar pressure, 
								uint32 time = B_NOW);

virtual	void	PitchBend(uchar channel, 
						  uchar lsb, 
						  uchar msb,
			    		  uint32 time = B_NOW);

virtual void	AllNotesOff(bool controlOnly, uint32 time = B_NOW);

protected:
	void*			fSongVariables;
	void*			fPerformanceVariables;
	bool			fMidiQueue;

private:

virtual	void		_ReservedMidiSynth1();
virtual	void		_ReservedMidiSynth2();
virtual	void		_ReservedMidiSynth3();
virtual	void		_ReservedMidiSynth4();

friend class BSynth;

	virtual	void		Run();
	bool fInputEnabled;
	uint32	_reserved[4];
};

#endif
