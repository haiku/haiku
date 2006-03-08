// AudioAdapterParams.h

#ifndef __AudioAdapterParams_H__
#define __AudioAdapterParams_H__

#include <MediaDefs.h>

#include "IParameterSet.h"

class _AudioAdapterParams :
	public	IParameterSet {
public:
	enum parameter_id {
		// input format restrictions (0='wildcard')
		P_INPUT_FORMAT					=101,
		P_INPUT_CHANNEL_COUNT,
		
		// output format restrictions (0='wildcard')
		P_OUTPUT_FORMAT					=201,
		P_OUTPUT_CHANNEL_COUNT
	};
	
public:
	_AudioAdapterParams() :
		inputFormat(media_raw_audio_format::wildcard),
		outputFormat(media_raw_audio_format::wildcard) {}
		
	status_t store(
		int32										parameterID,
		const void*							data,
		size_t									size);

	status_t retrieve(
		int32										parameterID,
		void*										data,
		size_t*									ioSize);

	void populateGroup(
		BParameterGroup* 				group);
		
public:											// accessible parameters

	media_multi_audio_format	inputFormat;
	media_multi_audio_format	outputFormat;
};

#endif /*__AudioAdapterParams_H__*/
