
#ifndef _MIDI_SYNTH_FILE_H
#define _MIDI_SYNTH_FILE_H

#include <BeBuild.h>
#include <MidiSynth.h>
#include <Entry.h>

typedef void (*synth_file_hook)(int32 arg);

class BMidiStore;

class BMidiSynthFile : public BMidiSynth
{
public:

	BMidiSynthFile();
	~BMidiSynthFile();

	status_t LoadFile(const entry_ref* midi_entry_ref);
	void UnloadFile(void);

	virtual status_t Start(void);
	virtual void Stop(void);

	void Fade(void);
	void Pause(void);
	void Resume(void);

	int32 Duration(void) const;
	int32 Position(int32 ticks) const;
	int32 Seek();

	status_t GetPatches(int16* pArray768, int16* pReturnedCount) const;

	void SetFileHook(synth_file_hook pSongHook, int32 arg);

	bool IsFinished(void) const;

	void ScaleTempoBy(double tempoFactor);
	void SetTempo(int32 newTempoBPM);
	int32 Tempo(void) const;

	void EnableLooping(bool loop);

	void MuteTrack(int16 track, bool do_mute);
	void GetMuteMap(char* pTracks) const;

	void SoloTrack(int16 track, bool do_solo);
	void GetSoloMap(char* pTracks) const;

private:

	friend class BSynth;							   

	virtual void _ReservedMidiSynthFile1();
	virtual void _ReservedMidiSynthFile2();
	virtual void _ReservedMidiSynthFile3();

	BMidiStore* fStore;
	
	int32 _reserved[3];
};

#endif // _MIDI_SYNTH_FILE
