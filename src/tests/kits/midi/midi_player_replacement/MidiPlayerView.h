/*
Author:	Jerome LEVEQUE
Email:	jerl1@caramail.com
*/
#ifndef MIDI_PLAYER_VIEW_H
#define MIDI_PLAYER_VIEW_H

#include <View.h>
#include <Box.h>

//----------------------------------------------------------
#define INPUT_CHANGE_TO_FILE 'icfi'
#define INPUT_CHANGE_TO_MIDIPORT 'icmp'
//--------------
#define OUTPUT_CHANGE_TO_FILE 'ocfi'
#define OUTPUT_CHANGE_TO_BEOS_SYNTH 'ocbs'
#define OUTPUT_CHANGE_TO_BEOS_SYNTH_FILE 'ocbf'
#define OUTPUT_CHANGE_TO_MIDIPORT 'ocmp'
//--------------
#define VIEW_CHANGE_TO_NONE 'vcno'
#define VIEW_CHANGE_TO_SCOPE 'vcsc'
#define VIEW_CHANGE_TO_ACTIVITY 'vcta'
//--------------
//--------------
//--------------
//--------------
//--------------
#define REWIND_INPUT_FILE 'reif'
#define PLAY_INPUT_FILE 'plif'
#define PAUSE_INPUT_FILE 'paif'
#define REWIND_OUTPUT_FILE 'reof'
#define SAVE_OUTPUT_FILE 'saof'
//--------------
#define CHANGE_INPUT_FILE 'chif'
#define CHANGE_OUTPUT_FILE 'chof'
#define CHANGE_INPUT_MIDIPORT 'chim'
#define CHANGE_OUTPUT_MIDIPORT 'chom'
//--------------
#define CHANGE_BEOS_SYNTH_FILE 'cbsf'
#define REWIND_BEOS_SYNTH_FILE 'rbsf'
#define PLAY_BEOS_SYNTH_FILE 'plsf'
#define PAUSE_BEOS_SYNTH_FILE 'pasf'
//--------------
#define CHANGE_SAMPLE_RATE_SYNTH 'csrs'
#define CHANGE_INTERPOLATION_TYPE_SYNTH 'cits'
#define CHANGE_REVERB_TYPE_SYNTH 'crts'
#define CHANGE_FILE_SAMPLE_SYNTH 'cfss'
#define CHANGE_VOLUME_SYNTH 'cvos'
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
#define BEOS_SYNTH_DIRECTORY "/boot/beos/etc/synth/"
//--------------

//----------------------------------------------------------

class MidiPlayerView : public BView
{
public:
	MidiPlayerView(void);
	~MidiPlayerView(void);
	
	virtual void AttachedToWindow(void);
	virtual	void MessageReceived(BMessage *msg);
	
	BStringView *GetInputStringView(void);
	BStringView *GetOutputStringView(void);

	void RemoveAll(BView *aView);
	void SetBeOSSynthView(BView *aView);
	
private:
	BBox *fInputBox;
	BBox *fOutputBox;
	BBox *fViewBox;
	
	BStringView *fInputStringView;
	BStringView *fOutputStringView;
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

#endif
