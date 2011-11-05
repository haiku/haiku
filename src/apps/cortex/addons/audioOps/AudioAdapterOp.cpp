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


// AudioAdapterOp.cpp

#include "AudioAdapterOp.h"
#include "IAudioOp.h"

#include "AudioAdapterParams.h"

#include "audio_buffer_tools.h"

#include <Debug.h>
#include <ParameterWeb.h>


//// empty parameter-set implementation
//// +++++ move to IParameterSet.h!
//
//class _EmptyParameterSet :
//	public	IParameterSet {
//public:
//	status_t store(
//		int32										parameterID,
//		void*										data,
//		size_t									size) { return B_ERROR; }
//
//	status_t retrieve(
//		int32										parameterID,
//		void*										data,
//		size_t*									ioSize) { return B_ERROR; }
//
//	void populateGroup(
//		BParameterGroup* 				group) {}
//};

// -------------------------------------------------------- //
// _AudioAdapterOp_base
// -------------------------------------------------------- //

class _AudioAdapterOp_base :
	public	IAudioOp {
public:
	_AudioAdapterOp_base(
		IAudioOpHost*						_host) :
		IAudioOp(_host) {}

	void replace(
		IAudioOp*								oldOp) {
		delete oldOp;
	}
};

// -------------------------------------------------------- //
// _AudioAdapterOp implementations
// -------------------------------------------------------- //

// direct conversion:
// - source and destination channel_count must be identical
// - source and destination must be host-endian

template <class in_t, class out_t>
class _AudioAdapterOp_direct :
	public	_AudioAdapterOp_base {

public:
	_AudioAdapterOp_direct(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_direct()\n"));
	}
		
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {
		
		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels <= 2);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == inChannels);

		bool stereo = (inChannels == 2);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;		
		while(frame--) {

			float val;
			convert_sample(
				*inBuffer,
				val);
			convert_sample(
				val,
				*outBuffer);
			
			++inBuffer;
			++outBuffer;

			if(stereo) {
				convert_sample(
					*inBuffer,
					val);			
				convert_sample(
					val,
					*outBuffer);
				++inBuffer;
				++outBuffer;
			}
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};

// direct conversion + incoming data byteswapped
// - source and destination channel_count must be identical
// - destination must be host-endian

template <class in_t, class out_t>
class _AudioAdapterOp_swap_direct :
	public	_AudioAdapterOp_base {

public:
	_AudioAdapterOp_swap_direct(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_swap_direct()\n"));
	}
		
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {
		
		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels <= 2);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == inChannels);

		bool stereo = (inChannels == 2);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;	
		while(frame--) {

			float val;
			swap_convert_sample(
				*inBuffer,
				val);
			convert_sample(
				val,
				*outBuffer);
			
			++inBuffer;
			++outBuffer;

			if(stereo) {
				swap_convert_sample(
					*inBuffer,
					val);
				convert_sample(
					val,
					*outBuffer);
				++inBuffer;
				++outBuffer;
			}
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};

template <class in_t, class out_t>
class _AudioAdapterOp_split :
	public	_AudioAdapterOp_base {
public:
	_AudioAdapterOp_split(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_split()\n"));
	}
		
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {

		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels == 1);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == 2);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;		
		while(frame--) {
		
			float val;
			convert_sample(
				*inBuffer,
				val);
			// write channel 0
			convert_sample(
				val,
				*outBuffer);

			// write channel 1
			++outBuffer;
			convert_sample(
				val,
				*outBuffer);
			
			++inBuffer;
			++outBuffer;
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};

template <class in_t, class out_t>
class _AudioAdapterOp_swap_split :
	public	_AudioAdapterOp_base {
public:
	_AudioAdapterOp_swap_split(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_swap_split()\n"));
	}
		
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {

		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels == 1);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == 2);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;		
		while(frame--) {
		
			float val;
			swap_convert_sample(
				*inBuffer,
				val);
			// write channel 0
			convert_sample(
				val,
				*outBuffer);

			// write channel 1
			++outBuffer;
			convert_sample(
				val,
				*outBuffer);
			
			++inBuffer;
			++outBuffer;
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};


template <class in_t, class out_t>
class _AudioAdapterOp_mix :
	public	_AudioAdapterOp_base {
public:
	_AudioAdapterOp_mix(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_mix()\n"));
	}
	
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {

		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels == 2);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == 1);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;
		while(frame--) {
		
			float out, in;
			convert_sample(
				*inBuffer,
				in);
				
			out = in * 0.5;
			++inBuffer;

			convert_sample(
				*inBuffer,
				in);

			out += (in * 0.5);
						
			// write channel 0
			convert_sample(
				out,
				*outBuffer);

			++inBuffer;
			++outBuffer;
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};

template <class in_t, class out_t>
class _AudioAdapterOp_swap_mix :
	public	_AudioAdapterOp_base {
public:
	_AudioAdapterOp_swap_mix(
		IAudioOpHost*						_host) :
		_AudioAdapterOp_base(_host) {
	
		PRINT(("### _AudioAdapterOp_swap_mix()\n"));
	}
		
	uint32 process(
		const AudioBuffer&			source,
		AudioBuffer&						destination,
		double&									sourceFrame,
		uint32&									destinationFrame,
		uint32									framesRequired,
		bigtime_t								performanceTime) {
		
		int32 inChannels = source.format().channel_count;
		ASSERT(inChannels == 2);
		int32 outChannels = destination.format().channel_count;
		ASSERT(outChannels == 1);
		
		in_t* inBuffer =
			((in_t*)source.data()) + (uint32)sourceFrame*inChannels;

		out_t* outBuffer =
			((out_t*)destination.data()) + destinationFrame*outChannels;

		uint32 frame = framesRequired;
		while(frame--) {
		
			float out, in;
			swap_convert_sample(
				*inBuffer,
				in);
				
			out = in * 0.5;
			++inBuffer;

			swap_convert_sample(
				*inBuffer,
				in);

			out += (in * 0.5);
						
			// write channel 0
			convert_sample(
				out,
				*outBuffer);

			++inBuffer;
			++outBuffer;
			
			sourceFrame += 1.0;
			destinationFrame++;
		}
		
		return framesRequired;
	}
};

// -------------------------------------------------------- //
// AudioAdapterOpFactory impl.
// -------------------------------------------------------- //

// [8sep99]  yeeechk! 
// [16sep99] now handles pre-conversion byteswapping

IAudioOp* AudioAdapterOpFactory::createOp(
	IAudioOpHost*										host,
	const media_raw_audio_format&		inputFormat,
	const media_raw_audio_format&		outputFormat) {

	// [16sep99] ensure fully-specified input & output formats
	ASSERT(
		inputFormat.frame_rate &&
		inputFormat.byte_order &&
		inputFormat.channel_count &&
		inputFormat.format &&
		inputFormat.buffer_size);
	ASSERT(
		outputFormat.frame_rate &&
		outputFormat.byte_order &&
		outputFormat.channel_count &&
		outputFormat.format &&
		outputFormat.buffer_size);

	int32 inChannels = inputFormat.channel_count;
	int32 outChannels = outputFormat.channel_count;

//	char fmt_buffer[256];
//	media_format f;
//	f.type = B_MEDIA_RAW_AUDIO;
//	f.u.raw_audio = inputFormat;
//	string_for_format(f, fmt_buffer, 255);

	bool swapBefore = (inputFormat.byte_order !=
		((B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN));

//	PRINT(("### swapBefore: '%s'\n", fmt_buffer));

	bool split = outChannels > inChannels;
	bool mix = outChannels < inChannels;

	switch(inputFormat.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			switch(outputFormat.format) {
				case media_raw_audio_format::B_AUDIO_UCHAR:
					return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < uint8, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < uint8, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < uint8, uint8>(host);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< uint8, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < uint8, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < uint8, short>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < uint8, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < uint8, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < uint8, short>(host);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< uint8, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < uint8, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < uint8, float>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < uint8, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < uint8, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < uint8, float>(host);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< uint8, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < uint8, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < uint8, int32>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < uint8, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < uint8, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < uint8, int32>(host);
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_SHORT:
			switch(outputFormat.format) {
				case media_raw_audio_format::B_AUDIO_UCHAR:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< short, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < short, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < short, uint8>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < short, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < short, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < short, uint8>(host);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< short, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < short, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < short, short>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < short, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < short, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < short, short>(host);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< short, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < short, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < short, float>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < short, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < short, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < short, float>(host);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< short, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < short, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < short, int32>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < short, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < short, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < short, int32>(host);
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_FLOAT:
			switch(outputFormat.format) {
				case media_raw_audio_format::B_AUDIO_UCHAR:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< float, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < float, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < float, uint8>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < float, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < float, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < float, uint8>(host);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< float, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < float, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < float, short>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < float, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < float, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < float, short>(host);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< float, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < float, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < float, float>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < float, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < float, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < float, float>(host);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< float, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < float, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < float, int32>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < float, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < float, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < float, int32>(host);
					break;
			}
			break;

		case media_raw_audio_format::B_AUDIO_INT:
			switch(outputFormat.format) {
				case media_raw_audio_format::B_AUDIO_UCHAR:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< int32, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < int32, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < int32, uint8>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < int32, uint8>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < int32, uint8>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < int32, uint8>(host);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< int32, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < int32, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < int32, short>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < int32, short>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < int32, short>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < int32, short>(host);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< int32, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < int32, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < int32, float>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < int32, float>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < int32, float>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < int32, float>(host);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					if(swapBefore) return
						split ? (IAudioOp*)new _AudioAdapterOp_swap_split< int32, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_swap_mix  < int32, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_swap_direct   < int32, int32>(host);
					else return
						split ? (IAudioOp*)new _AudioAdapterOp_split     < int32, int32>(host) :
							mix ? (IAudioOp*)new _AudioAdapterOp_mix       < int32, int32>(host) :
								(IAudioOp*)new _AudioAdapterOp_direct        < int32, int32>(host);
					break;
			}
			break;
	}
	
	return 0;
}
		
IParameterSet* AudioAdapterOpFactory::createParameterSet() {
	return new _AudioAdapterParams();
}


// END -- AudioAdapterOp.cpp --
