/*******************************************************************************
/
/	File:		MidiSynthFile.h
/
/	Description:	Send a MIDI file to the synthesizer.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MIDI_SYNTH_FILE_H
#define _MIDI_SYNTH_FILE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <MidiSynth.h>
#include <Entry.h>

typedef void (*synth_file_hook)(int32 arg);


class BMidiSynthFile : public BMidiSynth
{
public:
				BMidiSynthFile();
				~BMidiSynthFile();

	status_t	LoadFile(const entry_ref *midi_entry_ref);
	void		UnloadFile(void);

	virtual status_t	Start(void);
	virtual void		Stop(void);

	void 		Fade(void);

	void		Pause(void);
	void		Resume(void);

	/* get ticks in microseconds of length of song*/
	int32		Duration(void) const;
	/* set the current playback position of song in microseconds*/
	int32		Position(int32 ticks) const;
	/* get the current playback position of a song in microseconds*/
	int32		Seek();

	/* get the patches required to play this song*/
	status_t		GetPatches(int16 *pArray768, int16 *pReturnedCount) const;

	/* Set a call back when song is done*/
	void		SetFileHook(synth_file_hook pSongHook, int32 arg);
	/* poll to see if song is done*/
	bool		IsFinished(void) const;

	/* set song master tempo. (1.0 uses songs encoded tempo, 2.0 will play*/
	/* song twice as fast, and 0.5 will play song half as fast*/
	void		ScaleTempoBy(double tempoFactor);


	/* sets tempo in beats per minute*/
	void		SetTempo(int32 newTempoBPM);
	/* returns tempo in beats per minute*/
	int32		Tempo(void) const;

	/* pass true to loop song, false to not loop*/
	void		EnableLooping(bool loop);

	/* Mute and unmute tracks (0 to 64)*/
	void		MuteTrack(int16 track, bool do_mute);
	void		GetMuteMap(char *pTracks) const;

	void		SoloTrack(int16 track, bool do_solo);
	void		GetSoloMap(char *pTracks) const;

private:

virtual	void		_ReservedMidiSynthFile1();
virtual	void		_ReservedMidiSynthFile2();
virtual	void		_ReservedMidiSynthFile3();

friend class BSynth;							   

		uint32		_reserved[4];
};

#endif
