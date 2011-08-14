/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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