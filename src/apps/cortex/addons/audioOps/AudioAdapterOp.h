// AudioAdapterOp.h
// * PURPOSE
//   An IAudioOp/IAudioOpFactory implementation providing
//   simple raw-audio format conversion.
//
// * HISTORY
//   e.moon			8sep99		Begun

#ifndef __AudioAdapterOp_H__
#define __AudioAdapterOp_H__

#include "IAudioOpFactory.h"

class AudioAdapterOpFactory :
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

#endif /*__AudioAdapterOp_H__*/