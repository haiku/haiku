/*
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */


#include "MultiAudioDevice.h"

#include <errno.h>
#include <string.h>

#include <MediaDefs.h>

#include "debug.h"
#include "MultiAudioUtility.h"


using namespace MultiAudio;


MultiAudioDevice::MultiAudioDevice(const char* name, const char* path)
{
	CALLED();

	strlcpy(fPath, path, B_PATH_NAME_LENGTH);
	PRINT(("name: %s, path: %s\n", name, fPath));

	fInitStatus = _InitDriver();
}


MultiAudioDevice::~MultiAudioDevice()
{
	CALLED();
	if (fDevice >= 0)
		close(fDevice);
}


status_t
MultiAudioDevice::InitCheck() const
{
	CALLED();
	return fInitStatus;
}


status_t
MultiAudioDevice::BufferExchange(multi_buffer_info *info)
{
	return buffer_exchange(fDevice, info);
}


status_t
MultiAudioDevice::SetMix(multi_mix_value_info *info)
{
	return set_mix(fDevice, info);
}


status_t
MultiAudioDevice::GetMix(multi_mix_value_info *info)
{
	return get_mix(fDevice, info);
}


status_t
MultiAudioDevice::SetInputFrameRate(uint32 multiAudioRate)
{
	if ((fDescription.input_rates & multiAudioRate) == 0)
		return B_BAD_VALUE;

	if (fFormatInfo.input.rate == multiAudioRate)
		return B_OK;

	uint32 oldRate = fFormatInfo.input.rate;
	fFormatInfo.input.rate = multiAudioRate;

	status_t status = set_global_format(fDevice, &fFormatInfo);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_SET_GLOBAL_FORMAT: %s\n",
			strerror(status));
		fFormatInfo.input.rate = oldRate;
		return status;
	}

	return _GetBuffers();
}


status_t
MultiAudioDevice::SetOutputFrameRate(uint32 multiAudioRate)
{
	if ((fDescription.output_rates & multiAudioRate) == 0)
		return B_BAD_VALUE;

	if (fFormatInfo.output.rate == multiAudioRate)
		return B_OK;

	uint32 oldRate = fFormatInfo.output.rate;
	fFormatInfo.output.rate = multiAudioRate;

	status_t status = set_global_format(fDevice, &fFormatInfo);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_SET_GLOBAL_FORMAT: %s\n",
			strerror(status));
		fFormatInfo.output.rate = oldRate;
		return status;
	}

	return _GetBuffers();
}


status_t
MultiAudioDevice::_InitDriver()
{
	int num_outputs, num_inputs, num_channels;

	CALLED();

	// open the device driver

	fDevice = open(fPath, O_WRONLY);
	if (fDevice == -1) {
		fprintf(stderr, "Failed to open %s: %s\n", fPath, strerror(errno));
		return B_ERROR;
	}

	// Get description

	fDescription.info_size = sizeof(fDescription);
	fDescription.request_channel_count = MAX_CHANNELS;
	fDescription.channels = fChannelInfo;
	status_t status = get_description(fDevice, &fDescription);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_GET_DESCRIPTION: %s\n",
			strerror(status));
		return status;
	}

	PRINT(("Friendly name:\t%s\nVendor:\t\t%s\n",
		fDescription.friendly_name, fDescription.vendor_info));
	PRINT(("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
		fDescription.output_channel_count, fDescription.input_channel_count,
		fDescription.output_bus_channel_count,
		fDescription.input_bus_channel_count));
	PRINT(("\nChannels\n"
		"ID\tKind\tDesig\tConnectors\n"));

	for (int32 i = 0; i < fDescription.output_channel_count
			+ fDescription.input_channel_count; i++) {
		PRINT(("%ld\t%d\t0x%lx\t0x%lx\n", fDescription.channels[i].channel_id,
			fDescription.channels[i].kind,
			fDescription.channels[i].designations,
			fDescription.channels[i].connectors));
	}
	PRINT(("\n"));

	PRINT(("Output rates\t\t0x%lx\n", fDescription.output_rates));
	PRINT(("Input rates\t\t0x%lx\n", fDescription.input_rates));
	PRINT(("Max CVSR\t\t%.0f\n", fDescription.max_cvsr_rate));
	PRINT(("Min CVSR\t\t%.0f\n", fDescription.min_cvsr_rate));
	PRINT(("Output formats\t\t0x%lx\n", fDescription.output_formats));
	PRINT(("Input formats\t\t0x%lx\n", fDescription.input_formats));
	PRINT(("Lock sources\t\t0x%lx\n", fDescription.lock_sources));
	PRINT(("Timecode sources\t0x%lx\n", fDescription.timecode_sources));
	PRINT(("Interface flags\t\t0x%lx\n", fDescription.interface_flags));
	PRINT(("Control panel string:\t\t%s\n", fDescription.control_panel));
	PRINT(("\n"));

	num_outputs = fDescription.output_channel_count;
	num_inputs = fDescription.input_channel_count;
	num_channels = num_outputs + num_inputs;

	// Get and set enabled channels

	multi_channel_enable enable;
	uint32 enableBits;
	enable.info_size = sizeof(enable);
	enable.enable_bits = (uchar*)&enableBits;

	status = get_enabled_channels(fDevice, &enable);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_GET_ENABLED_CHANNELS: %s\n",
			strerror(status));
		return status;
	}

	enableBits = (1 << num_channels) - 1;
	enable.lock_source = B_MULTI_LOCK_INTERNAL;

	status = set_enabled_channels(fDevice, &enable);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%x: %s\n",
			*enable.enable_bits, strerror(status));
		return status;
	}

	// Set the sample rate

	fFormatInfo.info_size = sizeof(multi_format_info);
	fFormatInfo.output.rate = select_sample_rate(fDescription.output_rates);
	fFormatInfo.output.cvsr = 0;
	fFormatInfo.output.format = select_format(fDescription.output_formats);
	fFormatInfo.input.rate = select_sample_rate(fDescription.input_rates);
	fFormatInfo.input.cvsr = fFormatInfo.output.cvsr;
	fFormatInfo.input.format = select_format(fDescription.input_formats);

	status = set_global_format(fDevice, &fFormatInfo);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_SET_GLOBAL_FORMAT: %s\n",
			strerror(status));
	}

	status = get_global_format(fDevice, &fFormatInfo);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_GET_GLOBAL_FORMAT: %s\n",
			strerror(status));
		return status;
	}

	// Get the buffers
	status = _GetBuffers();
	if (status != B_OK)
		return status;


	fMixControlInfo.info_size = sizeof(fMixControlInfo);
	fMixControlInfo.control_count = MAX_CONTROLS;
	fMixControlInfo.controls = fMixControl;

	status = list_mix_controls(fDevice, &fMixControlInfo);
	if (status != B_OK) {
		fprintf(stderr, "Failed on DRIVER_LIST_MIX_CONTROLS: %s\n",
			strerror(status));
		return status;
	}

	return B_OK;
}


status_t
MultiAudioDevice::_GetBuffers()
{
	for (uint32 i = 0; i < MAX_BUFFERS; i++) {
		fPlayBuffers[i] = &fPlayBufferList[i * MAX_CHANNELS];
		fRecordBuffers[i] = &fRecordBufferList[i * MAX_CHANNELS];
	}
	fBufferList.info_size = sizeof(multi_buffer_list);
	fBufferList.request_playback_buffer_size = 0;
		// use the default...
	fBufferList.request_playback_buffers = MAX_BUFFERS;
	fBufferList.request_playback_channels = fDescription.output_channel_count;
	fBufferList.playback_buffers = (buffer_desc **) fPlayBuffers;
	fBufferList.request_record_buffer_size = 0;
		// use the default...
	fBufferList.request_record_buffers = MAX_BUFFERS;
	fBufferList.request_record_channels = fDescription.input_channel_count;
	fBufferList.record_buffers = /*(buffer_desc **)*/ fRecordBuffers;

	status_t status = get_buffers(fDevice, &fBufferList);
	if (status != B_OK) {
		fprintf(stderr, "Failed on B_MULTI_GET_BUFFERS: %s\n",
			strerror(status));
		return status;
	}

	for (int32 i = 0; i < fBufferList.return_playback_buffers; i++) {
		for (int32 j = 0; j < fBufferList.return_playback_channels; j++) {
			PRINT(("fBufferList.playback_buffers[%ld][%ld].base: %p\n",
				i, j, fBufferList.playback_buffers[i][j].base));
			PRINT(("fBufferList.playback_buffers[%ld][%ld].stride: %li\n",
				i, j, fBufferList.playback_buffers[i][j].stride));
		}
	}

	for (int32 i = 0; i < fBufferList.return_record_buffers; i++) {
		for (int32 j = 0; j < fBufferList.return_record_channels; j++) {
			PRINT(("fBufferList.record_buffers[%ld][%ld].base: %p\n",
				i, j, fBufferList.record_buffers[i][j].base));
			PRINT(("fBufferList.record_buffers[%ld][%ld].stride: %li\n",
				i, j, fBufferList.record_buffers[i][j].stride));
		}
	}

	return B_OK;
}
