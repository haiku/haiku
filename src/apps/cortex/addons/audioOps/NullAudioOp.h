// NullAudioOp.h
// * PURPOSE
//   To test the IAudioOp framework, this file includes
//   an IAudioOpFactory implementation that creates
//   'do-nothing' operations.  (This operation literally
//   does nothing -- it doesn't even copy the input buffer
//   to the output, so beware!)
//
// * HISTORY
//   e.moon		8sep99		Begun

#ifndef __NullAudioOp_H__
#define __NullAudioOp_H__

#include "IAudioOpFactory.h"

class NullAudioOpFactory :
	public	IAudioOpFactory {
public:											// *** INTERFACE
	// The basic create method.
	// Return 0 if no algorithm could be found for the given format.

	IAudioOp* createOp(
		IAudioOpHost*										host,
		const media_raw_audio_format&		inputFormat,
		const media_raw_audio_format&		outputFormat); //nyi
		
	IParameterSet* createParameterSet(); //nyi
};

#endif /*__NullAudioOp_H__*/