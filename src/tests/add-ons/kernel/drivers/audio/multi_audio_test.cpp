/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <hmulti_audio.h>

#include "argv.h"


#define MAX_CONTROLS	128
#define MAX_CHANNELS	32
#define NUM_BUFFERS		16

const uint32 kSampleRates[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100,
	48000, 64000, 88200, 96000, 176400, 192000, 384000, 1536000
};
const struct {
	uint32	type;
	const char*	name;
} kFormats[] = {
	{B_FMT_8BIT_S, "8bit"},
	{B_FMT_8BIT_U, "8bit-unsigned"},
	{B_FMT_16BIT, "16bit"},
	{B_FMT_18BIT, "18bit"},
	{B_FMT_20BIT, "20bit"},
	{B_FMT_24BIT, "24bit"},
	{B_FMT_32BIT, "32bit"},
	{B_FMT_FLOAT, "float"},
	{B_FMT_DOUBLE, "double"}
};


struct cmd_entry {
	const char*	name;
	void		(*func)(int argc, char **argv);
	const char*	help;
};


extern const char* __progname;

static void do_help(int argc, char** argv);


static int sDevice;
static multi_channel_info sChannelInfo[MAX_CHANNELS];
static multi_description sDescription;
static uint32 sRate = B_SR_48000;
static uint32 sFormat = B_FMT_32BIT;
static uint32 sEnabledChannels = ~0;


static uint32
channel_count()
{
	return sDescription.output_channel_count + sDescription.input_channel_count;
}


static void
set_frame(char* frame, uint32 format, float value)
{
	switch (format) {
		case B_FMT_8BIT_U:
			*(uint8*)frame = uint8(value * INT8_MAX) + 128;
			break;
		case B_FMT_8BIT_S:
			*(int8*)frame = int8(value * INT8_MAX);
			break;
		case B_FMT_16BIT:
			*(int16*)frame = int16(value * INT16_MAX);
			break;
		case B_FMT_18BIT:
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
			*(int32*)frame = int32(value * INT32_MAX);
			break;
		default:
			*(float*)frame = value;
			break;
	}
}


static uint32
get_rate(uint32 rateBits)
{
	uint32 rate = 0;
	for (uint32 i = 0; (1UL << i) <= rateBits
			&& i < sizeof(kSampleRates) / sizeof(kSampleRates[0]); i++) {
		if (((1 << i) & rateBits) != 0)
			rate = kSampleRates[i];
	}

	return rate;
}


static void
print_rates(uint32 rateBits)
{
	for (uint32 i = 0; i < sizeof(kSampleRates) / sizeof(kSampleRates[0]);
			i++) {
		if (((1 << i) & rateBits) != 0)
			printf("  %lu", kSampleRates[i]);
	}
	putchar('\n');
}


static const char*
get_format_name(uint32 format)
{
	for (uint32 i = 0; i < sizeof(kFormats) / sizeof(kFormats[0]); i++) {
		if (kFormats[i].type == format)
			return kFormats[i].name;
	}

	return "unknown";
}


static void
print_formats(uint32 formatBits)
{
	for (uint32 i = 0; i < sizeof(kFormats) / sizeof(kFormats[0]); i++) {
		if ((kFormats[i].type & formatBits) != 0)
			printf("  %s", kFormats[i].name);
	}
	putchar('\n');
}


static const char*
get_kind_name(uint32 kind)
{
	switch (kind) {
		case B_MULTI_OUTPUT_CHANNEL:
			return "out";
		case B_MULTI_INPUT_CHANNEL:
			return "in";
		case B_MULTI_OUTPUT_BUS:
			return "bus-out";
		case B_MULTI_INPUT_BUS:
			return "bus-in";
		case B_MULTI_AUX_BUS:
			return "bus-aux";

		default:
			return "unknown";
	}
}


//	#pragma mark - Commands


static void
do_rate(int argc, char** argv)
{
	if (argc > 1) {
		uint32 rate = strtoul(argv[1], NULL, 0);
		uint32 bits = 0;

		for (uint32 i = 0; i < sizeof(kSampleRates) / sizeof(kSampleRates[0]);
				i++) {
			if (rate == kSampleRates[i])
				bits = 1 << i;
		}

		if (bits == 0) {
			fprintf(stderr, "Invalid sample rate %ld!\n", rate);
			printf("Valid values are:");
			for (uint32 i = 0;
					i < sizeof(kSampleRates) / sizeof(kSampleRates[0]); i++) {
				printf(" %lu", kSampleRates[i]);
			}
			putchar('\n');
			return;
		}
		sRate = bits;
	}

	printf("Current sample rate is %lu Hz (0x%lx)\n", get_rate(sRate), sRate);
}


static void
do_format(int argc, char** argv)
{
	int32 i = -1;
	if (argc == 2)
		i = strtoll(argv[1], NULL, 0);

	if (i < 1 || i > (int32)(sizeof(kFormats) / sizeof(kFormats[0]))) {
		if (argc == 2)
			fprintf(stderr, "Invalid format: %ld\n", i);

		for (uint32 i = 0; i < sizeof(kFormats) / sizeof(kFormats[0]); i++) {
			printf("[%ld] %s\n", i + 1, kFormats[i].name);
		}
	} else {
		sFormat = kFormats[i - 1].type;
	}

	printf("Current sample format is %s (0x%lx)\n", get_format_name(sFormat),
		sFormat);
}


static void
do_desc(int argc, char** argv)
{
	printf("friendly name:\t\t\t%s\n", sDescription.friendly_name);
	printf("vendor:\t\t\t\t%s\n\n", sDescription.vendor_info);

	printf("output rates:\t\t\t0x%lx\n", sDescription.output_rates);
	print_rates(sDescription.output_rates);
	printf("input rates:\t\t\t0x%lx\n", sDescription.input_rates);
	print_rates(sDescription.input_rates);
	printf("max cont. var. sample rate:\t%.0f\n",
		sDescription.max_cvsr_rate);
	printf("min cont. var. sample rate:\t%.0f\n",
		sDescription.min_cvsr_rate);
	printf("output formats:\t\t\t0x%lx\n", sDescription.output_formats);
	print_formats(sDescription.output_formats);
	printf("input formats:\t\t\t0x%lx\n", sDescription.input_formats);
	print_formats(sDescription.input_formats);
	printf("lock sources:\t\t\t0x%lx\n", sDescription.lock_sources);
	printf("timecode sources:\t\t0x%lx\n", sDescription.timecode_sources);
	printf("interface flags:\t\t0x%lx\n", sDescription.interface_flags);
	printf("control panel string:\t\t\t%s\n", sDescription.control_panel);

	printf("\nchannels:\n");
	printf("  %ld outputs, %ld inputs\n", sDescription.output_channel_count,
		sDescription.input_channel_count);
	printf("  %ld out busses, %ld in busses\n",
		sDescription.output_bus_channel_count,
		sDescription.input_bus_channel_count);
	printf("\n  ID\tkind\t   designations\tconnectors\n");

	for (uint32 i = 0 ; i < channel_count(); i++) {
		printf("%4ld\t%-10s 0x%lx\t0x%lx\n", sDescription.channels[i].channel_id,
			get_kind_name(sDescription.channels[i].kind),
			sDescription.channels[i].designations,
			sDescription.channels[i].connectors);
	}
}


static void
do_channels(int argc, char** argv)
{
	if (argc == 2)
		sEnabledChannels = strtoul(argv[1], NULL, 0);

	uint32 enabled = ((1 << channel_count()) - 1) & sEnabledChannels;

	printf("%ld channels:\n  ", channel_count());
	for (uint32 i = 0; i < channel_count(); i++) {
		printf(enabled & 1 ? "x" : "-");
		enabled >>= 1;
	}
	putchar('\n');
}


static void
do_play(int argc, char** argv)
{
	uint32 playMask = 0xffffffff;
	if (argc == 2)
		playMask = strtoul(argv[1], NULL, 0);

	multi_channel_enable channelEnable;
	uint32 enabled = ((1 << channel_count()) - 1) & sEnabledChannels;

	channelEnable.enable_bits = (uchar*)&enabled;
	channelEnable.lock_source = B_MULTI_LOCK_INTERNAL;
	if (ioctl(sDevice, B_MULTI_SET_ENABLED_CHANNELS, &channelEnable,
			sizeof(multi_channel_enable)) < B_OK) {
		fprintf(stderr, "Setting enabled channels failed: %s\n",
			strerror(errno));
	}

	multi_format_info formatInfo;
	formatInfo.info_size = sizeof(multi_format_info);
	formatInfo.output.rate = sRate;
	formatInfo.output.cvsr = 0;
	formatInfo.output.format = sFormat;
	formatInfo.input.rate = formatInfo.output.rate;
	formatInfo.input.cvsr = formatInfo.output.cvsr;
	formatInfo.input.format = formatInfo.output.format;

	if (ioctl(sDevice, B_MULTI_SET_GLOBAL_FORMAT, &formatInfo,
			sizeof(multi_format_info)) < B_OK) {
		printf("Setting global format failed: %s\n", strerror(errno));
	}

	if (ioctl(sDevice, B_MULTI_GET_GLOBAL_FORMAT, &formatInfo,
			sizeof(multi_format_info)) < B_OK) {
		printf("Getting global format failed: %s\n", strerror(errno));
	}

	printf("format %s (0x%lx)\n", get_format_name(formatInfo.output.format),
		formatInfo.output.format);
	printf("sample rate %lu (0x%lx)\n", get_rate(formatInfo.output.rate),
		formatInfo.output.rate);

	buffer_desc playBuffers[NUM_BUFFERS * MAX_CHANNELS];
	buffer_desc recordBuffers[NUM_BUFFERS * MAX_CHANNELS];
	buffer_desc* playBufferDesc[NUM_BUFFERS];
	buffer_desc* recordBufferDesc[NUM_BUFFERS];

	for (uint32 i = 0; i < NUM_BUFFERS; i++) {
		playBufferDesc[i] = &playBuffers[i * MAX_CHANNELS];
		recordBufferDesc[i] = &recordBuffers[i * MAX_CHANNELS];
	}

	multi_buffer_list bufferList;
	bufferList.info_size = sizeof(multi_buffer_list);
	bufferList.request_playback_buffer_size = 0;
	bufferList.request_playback_buffers = NUM_BUFFERS;
	bufferList.request_playback_channels = sDescription.output_channel_count;
	bufferList.playback_buffers = (buffer_desc**)playBufferDesc;
	bufferList.request_record_buffer_size = 0;
	bufferList.request_record_buffers = NUM_BUFFERS;
	bufferList.request_record_channels = sDescription.input_channel_count;
	bufferList.record_buffers = (buffer_desc**)recordBufferDesc;

	if (ioctl(sDevice, B_MULTI_GET_BUFFERS, &bufferList,
			sizeof(multi_buffer_list)) < B_OK) {
		printf("Getting buffers failed: %s\n", strerror(errno));
		return;
	}

	printf("playback: buffer count %ld, channels %ld, buffer size %ld\n",
		bufferList.return_playback_buffers, bufferList.return_playback_channels,
		bufferList.return_playback_buffer_size);
	for (int32 channel = 0; channel < bufferList.return_playback_channels;
			channel++) {
		printf("  Channel %ld\n", channel);
		for (int32 i = 0; i < bufferList.return_playback_buffers; i++) {
			printf("    [%ld] buffer %p, stride %ld\n", i,
				bufferList.playback_buffers[i][channel].base,
				bufferList.playback_buffers[i][channel].stride);
		}
	}

	printf("record: buffer count %ld, channels %ld, buffer size %ld\n",
		bufferList.return_record_buffers, bufferList.return_record_channels,
		bufferList.return_record_buffer_size);

	multi_buffer_info bufferInfo;
	memset(&bufferInfo, 0, sizeof(multi_buffer_info));
	bufferInfo.info_size = sizeof(multi_buffer_info);

	bigtime_t startTime = system_time();
	uint32 exchanged = 0;
	int32 cycle = -1;
	uint32 x = 0;
	while (true) {
		if (system_time() - startTime > 1000000LL)
			break;

		if (ioctl(sDevice, B_MULTI_BUFFER_EXCHANGE, &bufferInfo,
				sizeof(multi_buffer_list)) < B_OK) {
			printf("Getting buffers failed: %s\n", strerror(errno));
			continue;
		}

		// fill buffer with data

		// Note: hmulti-audio drivers may actually return more than once
		// per buffer...
		if (cycle == bufferInfo.playback_buffer_cycle
			&& bufferList.return_playback_buffers != 1)
			continue;

		cycle = bufferInfo.playback_buffer_cycle;

		size_t stride = bufferList.playback_buffers[cycle][0].stride;
		for (int32 channel = 0; channel < bufferList.return_playback_channels;
				channel++) {
			if (((1 << channel) & playMask) == 0)
				continue;

			char* dest = bufferList.playback_buffers[cycle][channel].base;
			for (uint32 frame = 0;
					frame < bufferList.return_playback_buffer_size; frame++) {
				set_frame(dest, formatInfo.output.format, sin((x + frame) / 32.0));
				dest += stride;
			}
		}

		x += bufferList.return_playback_buffer_size;
		exchanged++;
	}

	printf("%ld buffers exchanged while playing (%lu frames played (%Ld)).\n",
		exchanged, x, bufferInfo.played_frames_count);

	// clear buffers

	for (int32 i = 0; i < bufferList.return_playback_buffers; i++) {
		size_t stride = bufferList.playback_buffers[i][0].stride;
		for (int32 channel = 0; channel < sDescription.output_channel_count;
				channel++) {
			char* dest = bufferList.playback_buffers[i][channel].base;
			for (uint32 frame = bufferList.return_playback_buffer_size;
					frame-- > 0; ) {
				set_frame(dest, formatInfo.output.format, 0);
				dest += stride;
			}
		}
	}

	if (ioctl(sDevice, B_MULTI_BUFFER_FORCE_STOP, NULL, 0) < B_OK) {
		printf("Stopping audio failed: %s\n", strerror(errno));
	}
}


static cmd_entry sBuiltinCommands[] = {
	{"rate", do_rate, "Set sample rate"},
	{"format", do_format, "Set sample format"},
	{"desc", do_desc, "Shows description"},
	{"channels", do_channels, "Shows enabled/disabled channels"},
	{"play", do_play, "Plays a tone"},
	{"help", do_help, "prints this help text"},
	{"quit", NULL, "exits the application"},
	{NULL, NULL, NULL},
};


static void
do_help(int argc, char** argv)
{
	printf("Available commands:\n");

	for (cmd_entry* command = sBuiltinCommands; command->name != NULL; command++) {
		printf("%8s - %s\n", command->name, command->help);
	}
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <device>\n", __progname);
		return 1;
	}

	// open driver

	sDevice = open(argv[1], O_RDWR);
	if (sDevice < 0) {
		fprintf(stderr, "%s: Could not open \"%s\": %s\n", __progname, argv[1],
			strerror(errno));
		return 1;
	}

	// get description

	memset(&sDescription, 0, sizeof(multi_description));
	sDescription.info_size = sizeof(multi_description);
	sDescription.request_channel_count = MAX_CHANNELS;
	sDescription.channels = sChannelInfo;

	if (ioctl(sDevice, B_MULTI_GET_DESCRIPTION, &sDescription,
			sizeof(multi_description)) < 0) {
		fprintf(stderr, "%s: Getting description failed: %s\n", __progname,
			strerror(errno));
		close(sDevice);
		return 1;
	}

	// get enabled channels

	multi_channel_enable channelEnable;
	uint32 enabled;

	channelEnable.info_size = sizeof(multi_channel_enable);
	channelEnable.enable_bits = (uchar*)&enabled;

	if (ioctl(sDevice, B_MULTI_GET_ENABLED_CHANNELS, &channelEnable,
			sizeof(channelEnable)) < B_OK) {
		fprintf(stderr, "Failed on B_MULTI_GET_ENABLED_CHANNELS: %s\n",
			strerror(errno));
		return 1;
	}

	sEnabledChannels = enabled;

	while (true) {
		printf("> ");
		fflush(stdout);

		char line[1024];
		if (fgets(line, sizeof(line), stdin) == NULL)
			break;

        argc = 0;
        argv = build_argv(line, &argc);
        if (argv == NULL || argc == 0)
            continue;

        int length = strlen(argv[0]);

		if (!strcmp(argv[0], "quit")
			|| !strcmp(argv[0], "exit")
			|| !strcmp(argv[0], "q"))
			break;

		bool found = false;

		for (cmd_entry* command = sBuiltinCommands; command->name != NULL; command++) {
			if (!strncmp(command->name, argv[0], length)) {
				command->func(argc, argv);
				found = true;
				break;
			}
		}

		if (!found)
			fprintf(stderr, "Unknown command \"%s\". Type \"help\" for a list of commands.\n", argv[0]);

		free(argv);
	}

	close(sDevice);
	return 0;
}

