// IOStructures.h 
/*
	
	Structures to represent IO channels in the AudioMixer node
	By David Shipman, 2002

*/

#ifndef _IO_STRUCT_H
#define _IO_STRUCT_H

class mixer_input
{

	public:
	
							mixer_input(media_input &input);
							~mixer_input();

		media_input			fInput;
		int32				fProducerDataStatus; // status of upstream data
		bool				enabled; // there is a buffer ready for mixing
		char			*	fData;
		
		float			*	fGainScale; // multiplier for gain - computed at paramchange
		float			*	fGainDisplay; // the value that will be displayed for gain
		bigtime_t			fGainDisplayLastChange;
		
		
		int					fMuteValue; // the value of the 'mute' control
		bigtime_t			fMuteValueLastChange;
		
		float				fPanValue; // value of 'pan' control (only if mono)
		bigtime_t			fPanValueLastChange;	
		
		
		size_t				fEventOffset; // offset (in bytes) of the start of the next
											// _output_ buffer - use this for keeping in sync
		size_t				fDataSize; // size of ringbuffer
		
};

#endif;