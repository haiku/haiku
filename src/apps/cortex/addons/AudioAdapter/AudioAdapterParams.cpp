// AudioAdapterParams.cpp

#include "AudioAdapterParams.h"

#include <Debug.h>
#include <ParameterWeb.h>

status_t _AudioAdapterParams::store(
	int32										parameterID,
	const void*							data,
	size_t									size) {
	
	if(size < sizeof(int32))
		return B_NO_MEMORY;
	
	const uint32 d = *(uint32*)data;
	
	switch(parameterID) {
		// input format restrictions (0='wildcard')
		case P_INPUT_FORMAT:
			inputFormat.format = d;
			break;
			
		case P_INPUT_CHANNEL_COUNT:
			inputFormat.channel_count = d;
			break;
		
		// output format restrictions (0='wildcard')
		case P_OUTPUT_FORMAT:
			outputFormat.format = d;
			break;

		case P_OUTPUT_CHANNEL_COUNT:
			outputFormat.channel_count = d;
			break;

		default:
			return B_BAD_INDEX;
	}
	
	return B_OK;
}

status_t _AudioAdapterParams::retrieve(
	int32										parameterID,
	void*										data,
	size_t*									ioSize) {
	
	if(*ioSize < sizeof(int32)) {
		*ioSize = sizeof(int32);
		return B_NO_MEMORY;
	}
	
	switch(parameterID) {
		// input format restrictions (0='wildcard')
		case P_INPUT_FORMAT:
			*(uint32*)data = inputFormat.format;
			break;
			
		case P_INPUT_CHANNEL_COUNT:
			*(uint32*)data = inputFormat.channel_count;
			break;
		
		// output format restrictions (0='wildcard')
		case P_OUTPUT_FORMAT:
			*(uint32*)data = outputFormat.format;
			PRINT(("P_OUTPUT_FORMAT retrieved\n")); //+++++
			break;

		case P_OUTPUT_CHANNEL_COUNT:
			*(uint32*)data = outputFormat.channel_count;
			break;

		default:
			return B_BAD_INDEX;
	}
	
	return B_OK;
}

void _AudioAdapterParams::populateGroup(
	BParameterGroup* 				group) {
	
	BParameterGroup* inputGroup = group->MakeGroup("Input Format");
	
	BNullParameter* groupName;
	BDiscreteParameter* param;

	groupName = inputGroup->MakeNullParameter(
		0, B_MEDIA_NO_TYPE, "Input Format", B_GENERIC);

	param = inputGroup->MakeDiscreteParameter(
		P_INPUT_FORMAT,
		B_MEDIA_NO_TYPE,
		"Sample Format",
		B_GENERIC);
	param->AddItem(
		0,
		"*");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_FLOAT,
		"float");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_SHORT,
		"short");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_INT,
		"int32");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_UCHAR,
		"uint8");
	
	param = inputGroup->MakeDiscreteParameter(
		P_INPUT_CHANNEL_COUNT,
		B_MEDIA_NO_TYPE,
		"Channels",
		B_GENERIC);
	param->AddItem(
		0,
		"*");
	param->AddItem(
		1,
		"mono");
	param->AddItem(
		2,
		"stereo");
	param->AddItem(
		4,
		"4");
	param->AddItem(
		8,
		"8");

	BParameterGroup* outputGroup = group->MakeGroup("Output Format");

	groupName = outputGroup->MakeNullParameter(
		0, B_MEDIA_NO_TYPE, "Output Format", B_GENERIC);

	param = outputGroup->MakeDiscreteParameter(
		P_OUTPUT_FORMAT,
		B_MEDIA_NO_TYPE,
		"Sample Format",
		B_GENERIC);
	param->AddItem(
		0,
		"*");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_FLOAT,
		"float");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_SHORT,
		"short");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_INT,
		"int32");
	param->AddItem(
		media_multi_audio_format::B_AUDIO_UCHAR,
		"uint8");
	
	param = outputGroup->MakeDiscreteParameter(
		P_OUTPUT_CHANNEL_COUNT,
		B_MEDIA_NO_TYPE,
		"Channels",
		B_GENERIC);
	param->AddItem(
		0,
		"*");
	param->AddItem(
		1,
		"mono");
	param->AddItem(
		2,
		"stereo");
	param->AddItem(
		4,
		"4");
	param->AddItem(
		8,
		"8");
}

// END -- AudioAdapterParams.cpp

