// AudioAdapterParams.h
#ifndef AUDIO_ADAPTER_PARAMS_H
#define AUDIO_ADAPTER_PARAMS_H


#include "IParameterSet.h"

#include <MediaDefs.h>


class _AudioAdapterParams : public IParameterSet {
	public:
		enum parameter_id {
			// input format restrictions (0 = 'wildcard')
			P_INPUT_FORMAT = 101,
			P_INPUT_CHANNEL_COUNT,

			// output format restrictions (0 = 'wildcard')
			P_OUTPUT_FORMAT = 201,
			P_OUTPUT_CHANNEL_COUNT
		};

	public:
		_AudioAdapterParams()
			:
			inputFormat(media_raw_audio_format::wildcard),
			outputFormat(media_raw_audio_format::wildcard)
			{}

		status_t store(int32 parameterID, const void* data, size_t size);
		status_t retrieve(int32 parameterID, void* data, size_t* ioSize);
		void populateGroup(BParameterGroup* group);

	public:	// accessible parameters
		media_multi_audio_format inputFormat;
		media_multi_audio_format outputFormat;
};

#endif	// AUDIO_ADAPTER_PARAMS_H
