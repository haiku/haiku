// IOStructures.h 
/*
	
	Structures to represent IO channels in the AudioMixer node
	By David Shipman, 2002

*/

#ifndef _IO_STRUCT_H
#define _IO_STRUCT_H

struct input_channel
{

	public:

		media_input			fInput;
		int32				fProducerDataStatus; // status of upstream data
		bool				DataAvailable; // there is a buffer ready for mixing
		char			*	fData;

		size_t				fEventOffset; // offset (in bytes)
		size_t				fDataSize; // size of ringbuffer
		
};

#endif;
