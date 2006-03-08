// NullAudioOp.cpp

#include "NullAudioOp.h"
#include "IAudioOp.h"
#include "IParameterSet.h"

#include <Debug.h>
#include <ParameterWeb.h>

// -------------------------------------------------------- //
// _NullAudioOp
// -------------------------------------------------------- //

class _NullAudioOp :
	public	IAudioOp {
public:
	_NullAudioOp(
		IAudioOpHost*						_host) :
		IAudioOp(_host) {}
	
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceOffset,
		uint32&									destinationOffset,
		uint32									framesRequired,
		bigtime_t								performanceTime) {
		
		return framesRequired;
	}
	
	void replace(
		IAudioOp*								oldOp) {
		delete oldOp;
	}
};

// -------------------------------------------------------- //
// _NullParameterSet
// -------------------------------------------------------- //

class _NullParameterSet :
	public	IParameterSet {
public:
	status_t store(
		int32										parameterID,
		void*										data,
		size_t									size) { return B_ERROR; }

	status_t retrieve(
		int32										parameterID,
		void*										data,
		size_t*									ioSize) { return B_ERROR; }
		
	// implement this hook to return a BParameterGroup representing
	// the parameters represented by this set
	
	void populateGroup(
		BParameterGroup* 				group) {
//	
//		group->MakeNullParameter(
//			0,
//			B_MEDIA_NO_TYPE,
//			"NullFilter has no parameters",
//			B_GENERIC);	
	}
};

// -------------------------------------------------------- //
// NullAudioOpFactory impl.
// -------------------------------------------------------- //

IAudioOp* NullAudioOpFactory::createOp(
	IAudioOpHost*										host,
	const media_raw_audio_format&		inputFormat,
	const media_raw_audio_format&		outputFormat) {

	return new _NullAudioOp(host);	
}
		
IParameterSet* NullAudioOpFactory::createParameterSet() {
	return new _NullParameterSet();
}


// END -- NullAudioOp.cpp --