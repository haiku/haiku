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


// AudioAdapterParams.cpp

#include "AudioAdapterParams.h"

#include <Debug.h>
#include <ParameterWeb.h>

status_t
_AudioAdapterParams::store(int32 parameterID, const void* data, size_t size)
{
	if (size < sizeof(int32))
		return B_NO_MEMORY;

	const uint32 d = *(uint32*)data;

	switch (parameterID) {
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
		"Sample format:",
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
		"Channels:",
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
		"Sample format:",
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
		"Channels:",
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

