/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */


#include "ice1712_reg.h"
#include "io.h"
#include "multi.h"
#include "util.h"

#include <string.h>
#include "debug.h"

status_t ice1712Settings_apply(ice1712 *card);

static void ice1712Buffer_Start(ice1712 *card);
static uint32 ice1712UI_GetCombo(ice1712 *card, uint32 index);
static void ice1712UI_SetCombo(ice1712 *card, uint32 index, uint32 value);
static uint32 ice1712UI_GetOutput(ice1712 *card, uint32 index);
static void ice1712UI_SetOutput(ice1712 *card, uint32 index, uint32 value);
static void ice1712UI_GetVolume(ice1712 *card, multi_mix_value *mmv);
static void ice1712UI_SetVolume(ice1712 *card, multi_mix_value *mmv);
static void ice1712UI_CreateOutput(ice1712 *card, multi_mix_control **p_mmc,
	int32 output, int32 parent);
static void ice1712UI_CreateCombo(multi_mix_control **p_mmc,
	const char *values[], int32 parent, int32 nb_combo, const char *name);
static void ice1712UI_CreateChannel(multi_mix_control **p_mmc,
	int32 channel, int32 parent, const char* name);
static int32 ice1712UI_CreateGroup(multi_mix_control **p_mmc,
	int32 index, int32 parent, enum strind_id string, const char* name);
static int32 nb_control_created;

#define AUTHORIZED_RATE (B_SR_SAME_AS_INPUT | B_SR_96000 \
	| B_SR_88200 | B_SR_48000 | B_SR_44100)
#define AUTHORIZED_SAMPLE_SIZE (B_FMT_24BIT)

#define MAX_CONTROL	32

//ICE1712 Multi - Buffer
//----------------------

void
ice1712Buffer_Start(ice1712 *card)
{
	uint16 size = card->buffer_size * MAX_DAC;

	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

	write_mt_uint32(card, MT_PROF_PB_DMA_BASE_ADDRESS,
		(uint32)(card->phys_pb.address));
	write_mt_uint16(card, MT_PROF_PB_DMA_COUNT_ADDRESS,
		(size * SWAPPING_BUFFERS) - 1);
	//We want interrupt only from playback
	write_mt_uint16(card, MT_PROF_PB_DMA_TERM_COUNT, size - 1);
	ITRACE("SIZE DMA PLAYBACK %#x\n", size);

	size = card->buffer_size * MAX_ADC;

	write_mt_uint32(card, MT_PROF_REC_DMA_BASE_ADDRESS,
		(uint32)(card->phys_rec.address));
	write_mt_uint16(card, MT_PROF_REC_DMA_COUNT_ADDRESS,
		(size * SWAPPING_BUFFERS) - 1);
	//We do not want any interrupt from the record
	write_mt_uint16(card, MT_PROF_REC_DMA_TERM_COUNT, 0);
	ITRACE("SIZE DMA RECORD %#x\n", size);

	//Enable output AND Input from Analog CODEC
	switch (card->config.product) {
	//TODO: find correct value for all card
		case ICE1712_SUBDEVICE_DELTA66:
		case ICE1712_SUBDEVICE_DELTA44:
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
		case ICE1712_SUBDEVICE_DELTADIO2496:
		case ICE1712_SUBDEVICE_DELTA410:
		case ICE1712_SUBDEVICE_DELTA1010LT:
		case ICE1712_SUBDEVICE_DELTA1010:
			codec_write(card, AK45xx_CLOCK_FORMAT_REGISTER, 0x69);
			codec_write(card, AK45xx_RESET_REGISTER, 0x03);
			break;
		case ICE1712_SUBDEVICE_VX442:
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_1);
			break;
	}

	//Set Data Format for SPDif codec
	switch (card->config.product) {
	//TODO: find correct value for all card
		case ICE1712_SUBDEVICE_DELTA1010:
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496:
			break;
		case ICE1712_SUBDEVICE_DELTA66:
		case ICE1712_SUBDEVICE_DELTA44:
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_1);
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
			spdif_write(card, CS84xx_SERIAL_INPUT_FORMAT_REG, 0x85);
			spdif_write(card, CS84xx_SERIAL_OUTPUT_FORMAT_REG, 0x85);
//			spdif_write(card, CS84xx_SERIAL_OUTPUT_FORMAT_REG, 0x41);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT:
//			ak45xx_write_gpio(ice, reg_addr, data,
//				DELTA1010LT_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data,
//				DELTA1010LT_CODEC_CS_1);
//			ak45xx_write_gpio(ice, reg_addr, data,
//				DELTA1010LT_CODEC_CS_2);
//			ak45xx_write_gpio(ice, reg_addr, data,
//				DELTA1010LT_CODEC_CS_3);
			break;
		case ICE1712_SUBDEVICE_VX442:
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_1);
			break;
	}

	card->buffer = 1;
	write_mt_uint8(card, MT_PROF_PB_CONTROL, 5);
}


status_t
ice1712Buffer_Exchange(ice1712 *card, multi_buffer_info *data)
{
	multi_buffer_info buffer_info;

#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	buffer_info.flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	if (acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT
		| B_CAN_INTERRUPT, 50000) == B_TIMED_OUT) {
		ITRACE("buffer_exchange timeout\n");
	};

	// Playback buffers info
	buffer_info.played_real_time = card->played_time;
	buffer_info.played_frames_count = card->frames_count;
	buffer_info.playback_buffer_cycle = (card->buffer - 1)
		% SWAPPING_BUFFERS; //Buffer played

	// Record buffers info
	buffer_info.recorded_real_time = card->played_time;
	buffer_info.recorded_frames_count = card->frames_count;
	buffer_info.record_buffer_cycle = (card->buffer - 1)
		% SWAPPING_BUFFERS; //Buffer filled

#ifdef __HAIKU__
	if (user_memcpy(data, &buffer_info, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(data, &buffer_info, sizeof(buffer_info));
#endif

	return B_OK;
}


status_t
ice1712Buffer_Stop(ice1712 *card)
{
	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

	card->played_time = 0;
	card->frames_count = 0;
	card->buffer = 0;

	return B_OK;
}

//ICE1712 Multi - Description
//---------------------------

status_t
ice1712Get_Description(ice1712 *card, multi_description *data)
{
	int chan = 0, i, size;

	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	switch (card->config.product) {
		case ICE1712_SUBDEVICE_DELTA1010:
			strncpy(data->friendly_name, "Delta 1010", 32);
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496:
			strncpy(data->friendly_name, "Delta DIO 2496", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA66:
			strncpy(data->friendly_name, "Delta 66", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA44:
			strncpy(data->friendly_name, "Delta 44", 32);
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
			strncpy(data->friendly_name, "Audiophile 2496", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
			strncpy(data->friendly_name, "Delta 410", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT:
			strncpy(data->friendly_name, "Delta 1010 LT", 32);
			break;
		case ICE1712_SUBDEVICE_VX442:
			strncpy(data->friendly_name, "VX 442", 32);
			break;

		default:
			strncpy(data->friendly_name, "Unknow device", 32);
			break;
	}

	strncpy(data->vendor_info, "Haiku", 32);

	data->output_channel_count = card->total_output_channels;
	data->input_channel_count = card->total_input_channels;
	data->output_bus_channel_count = 0;
	data->input_bus_channel_count = 0;
	data->aux_bus_channel_count = 0;

	size =	data->output_channel_count + data->input_channel_count
		+ data->output_bus_channel_count + data->input_bus_channel_count
		+ data->aux_bus_channel_count;

	ITRACE_VV("request_channel_count = %" B_PRIi32 "\n",
		data->request_channel_count);

	if (size <= data->request_channel_count) {
		for (i = 0; i < card->config.nb_DAC; i++) {
		//Analog STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| (((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT);
			data->channels[chan].connectors = 0;
			chan++;
		}

		if (card->config.spdif & SPDIF_OUT_PRESENT) {
		//SPDIF STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| B_CHANNEL_LEFT;
			data->channels[chan].connectors = 0;
			chan++;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		for (i = 0; i < card->config.nb_ADC; i++) {
		//Analog STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| (((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT);
			data->channels[chan].connectors = 0;
			chan++;
		}

		if (card->config.spdif & SPDIF_IN_PRESENT) {
		//SPDIF STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| B_CHANNEL_LEFT;
			data->channels[chan].connectors = 0;
			chan++;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		//The digital mixer output (it's an Input for Haiku)
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS
			| B_CHANNEL_LEFT;
		data->channels[chan].connectors = 0;
		chan++;
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS
			| B_CHANNEL_RIGHT;
		data->channels[chan].connectors = 0;
		chan++;
	}

	ITRACE("output_channel_count = %" B_PRIi32 "\n",
		data->output_channel_count);
	ITRACE("input_channel_count = %" B_PRIi32 "\n",
		data->input_channel_count);
	ITRACE("output_bus_channel_count = %" B_PRIi32 "\n",
		data->output_bus_channel_count);
	ITRACE("input_bus_channel_count = %" B_PRIi32 "\n",
		data->input_bus_channel_count);

	data->output_rates = data->input_rates = AUTHORIZED_RATE;
	data->min_cvsr_rate = 44100;
	data->max_cvsr_rate = 96000;

	data->output_formats = data->input_formats = AUTHORIZED_SAMPLE_SIZE;
	data->lock_sources = B_MULTI_LOCK_INTERNAL | B_MULTI_LOCK_SPDIF;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK
		| B_MULTI_INTERFACE_RECORD;
	data->start_latency = 0;

	strcpy(data->control_panel,"");

	return B_OK;
}


status_t
ice1712Get_Channel(ice1712 *card, multi_channel_enable *data)
{
	int i, total_channel;
	uint8 reg;

	total_channel = card->total_output_channels + card->total_input_channels;
	for (i = 0; i < total_channel; i++)
		B_SET_CHANNEL(data->enable_bits, i, true);

	reg = read_mt_uint8(card, MT_SAMPLING_RATE_SELECT);

	if (reg == 0x10)
		data->lock_source = B_MULTI_LOCK_SPDIF;
	else
		data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}


status_t
ice1712Set_Channel(ice1712 *card, multi_channel_enable *data)
{
	int i;
	int total_channel;

	total_channel = card->total_output_channels + card->total_input_channels;
	for (i = 0; i < total_channel; i++)
		ITRACE_VV("set_enabled_channels %d : %s\n", i,
			B_TEST_CHANNEL(data->enable_bits, i) ? "enabled": "disabled");

	ITRACE_VV("lock_source %" B_PRIx32 "\n", data->lock_source);
	ITRACE_VV("lock_data %" B_PRIx32 "\n", data->lock_data);

	if (data->lock_source == B_MULTI_LOCK_SPDIF)
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT, 0x10);
	else
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT,
			card->config.samplingRate);

	card->config.lockSource = data->lock_source;

	return B_OK;
}


status_t
ice1712Get_Format(ice1712 *card, multi_format_info *data)
{
	uint8 sr = read_mt_uint8(card, MT_SAMPLING_RATE_SELECT);

	switch (sr) {
		case ICE1712_SAMPLERATE_48K:
			data->input.rate = data->output.rate = B_SR_48000;
			data->input.cvsr = data->output.cvsr = 48000.0f;
			break;
		case ICE1712_SAMPLERATE_96K:
			data->input.rate = data->output.rate = B_SR_96000;
			data->input.cvsr = data->output.cvsr = 96000.0f;
			break;
		case ICE1712_SAMPLERATE_44K1:
			data->input.rate = data->output.rate = B_SR_44100;
			data->input.cvsr = data->output.cvsr = 44100.0f;
			break;
		case ICE1712_SAMPLERATE_88K2:
			data->input.rate = data->output.rate = B_SR_88200;
			data->input.cvsr = data->output.cvsr = 88200.0f;
			break;
	}

	data->timecode_kind = 0;
	data->output_latency = data->input_latency = 0;
	data->output.format = data->input.format = AUTHORIZED_SAMPLE_SIZE;

	ITRACE("Sampling Rate = %f\n", data->input.cvsr);

	return B_OK;
}


status_t
ice1712Set_Format(ice1712 *card, multi_format_info *data)
{
	ITRACE("Input Sampling Rate = %" B_PRIu32 "\n",
		data->input.rate);
	ITRACE("Output Sampling Rate = %" B_PRIu32 "\n",
		data->output.rate);

	//We can't have a different rate for input and output
	//so just wait to change our sample rate when
	//media server will do what we need
	//Lie to it and say we are living in wonderland
	if (data->input.rate != data->output.rate)
		return B_OK;

	if (card->config.lockSource == B_MULTI_LOCK_INTERNAL) {
		switch (data->output.rate) {
			case B_SR_96000:
				card->config.samplingRate = 0x07;
				break;
			case B_SR_88200:
				card->config.samplingRate = 0x0B;
				break;
			case B_SR_48000:
				card->config.samplingRate = 0x00;
				break;
			case B_SR_44100:
				card->config.samplingRate = 0x08;
				break;
		}
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT,
			card->config.samplingRate);
	}
	ITRACE("New rate = %#x\n", read_mt_uint8(card, MT_SAMPLING_RATE_SELECT));

	return B_OK;
}

//ICE1712 Multi - UI
//------------------

static const char *Clock[] = {
	"Internal",
	"Digital",
	NULL,
};

static const char *DigitalFormat[] = {
	"Consumer",
	"Professional",
	NULL,
};

static const char *DigitalEmphasis[] = {
	"None",
	"CCITT",
	"15/50usec",
	NULL,
};

static const char *DigitalCopyMode[] = {
	"Original",
	"1st Generation",
	"No SCMS",
	NULL,
};

static const char **SettingsGeneral[] = {
	Clock,
	NULL,
};

static const char **SettingsDigital[] = {
	DigitalFormat,
	DigitalEmphasis,
	DigitalCopyMode,
	NULL,
};

static const char *string_list[] = {
	"Setup",
	"General",
	"Digital",
	"Output Selection",

	"Internal Mixer",

	//General settings
	"Master clock",
	"reserved_0",
	"reserved_1",

	//Digital settings
	"Output format",
	"Emphasis",
	"Copy mode",

	//Output Selection
	"Output 1", //11
	"Output 2",
	"Output 3",
	"Output 4",
	"Digital Output", //15

	"Haiku output", //16

	"Input 1", //17
	"Input 2",
	"Input 3",
	"Input 4",
	"Digital Input", //21
	"Internal mixer", //22
};


/*
 * This will create a Tab
 */
int32
ice1712UI_CreateGroup(multi_mix_control **p_mmc, int32 index,
	int32 parent, enum strind_id string, const char* name)
{
	multi_mix_control *mmc = *p_mmc;
	int32 group;

	mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + ICE1712_MULTI_SET_INDEX(index);
	mmc->parent = parent;
	mmc->flags = B_MULTI_MIX_GROUP;
	mmc->master = CONTROL_IS_MASTER;
	mmc->string = string;

	group = mmc->id;

	if (name != NULL)
		strcpy(mmc->name, name);

	ITRACE_VV("Create Group: ID %#" B_PRIx32 "\n", mmc->id);

	nb_control_created++; mmc++;
	(*p_mmc) = mmc;

	return group;
}


/*
 * This will create a Slider with a "Mute" CheckBox
 */
void
ice1712UI_CreateChannel(multi_mix_control **p_mmc, int32 channel,
	int32 parent, const char* name)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_VOLUME
		+ ICE1712_MULTI_SET_CHANNEL(channel);
	multi_mix_control *mmc = *p_mmc;
	multi_mix_control control;

	control.master = CONTROL_IS_MASTER;
	control.parent = parent;
	control.gain.max_gain = 0.0;
	control.gain.min_gain = -144.0;
	control.gain.granularity = 1.5;

	//The Mute Checkbox
	control.id = id++;
	control.flags = B_MULTI_MIX_ENABLE;
	control.string = S_MUTE;
	*mmc = control;
	mmc++;

	ITRACE_VV("Create Channel (Mute): ID %#" B_PRIx32 "\n", control.id);

	//The Left Slider
	control.string = S_null;
	control.id = id++;
	control.flags = B_MULTI_MIX_GAIN;
	if (name != NULL)
		strcpy(control.name, name);
	*mmc = control;
	mmc++;

	ITRACE_VV("Create Channel (Left): ID %#" B_PRIx32 "\n", control.id);

	//The Right Slider
	control.master = control.id; //The Id of the Left Slider
	control.id = id++;
	*mmc = control;
	mmc++;

	ITRACE_VV("Create Channel (Right): ID %#" B_PRIx32 "\n", control.id);

	nb_control_created += 3;
	(*p_mmc) = mmc;
}


void
ice1712UI_CreateCombo(multi_mix_control **p_mmc, const char *values[],
	int32 parent, int32 nb_combo, const char *name)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_COMBO
		+ ICE1712_MULTI_SET_CHANNEL(nb_combo);
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	//The label
	parentControl = mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, name);

	ITRACE_VV("Create Combo (label): ID %#" B_PRIx32 "\n", parentControl);

	nb_control_created++; mmc++;

	//The values
	for (i = 0; values[i] != NULL; i++) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, values[i]);

		ITRACE_VV("Create Combo (value): ID %#" B_PRIx32 "\n", mmc->id);

		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


/*
 * This will create all possible value for the output
 * output 0 -> 3 (physical stereo output) 4 is the Digital
 */
void
ice1712UI_CreateOutput(ice1712 *card, multi_mix_control **p_mmc,
	int32 output, int32 parent)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_OUTPUT
		+ ICE1712_MULTI_SET_CHANNEL(output);
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	//The label
	parentControl = mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, string_list[11 + output]);
	nb_control_created++; mmc++;

	ITRACE_VV("Create Output (label): ID %#" B_PRIx32 "\n", parentControl);

	//Haiku output
	mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX_VALUE;
	mmc->parent = parentControl;
	strcpy(mmc->name, string_list[16]);

	ITRACE_VV("Create Output (Haiku): ID %#" B_PRIx32 "\n", mmc->id);

	nb_control_created++; mmc++;

	//Physical Input
	for (i = 0; i < card->config.nb_DAC; i += 2) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[17 + (i / 2)]);

		ITRACE_VV("Create Output (Physical In): ID %#" B_PRIx32 "\n", mmc->id);

		nb_control_created++; mmc++;
	}

	//Physical Digital Input
	if (card->config.spdif & SPDIF_IN_PRESENT) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[21]);

		ITRACE_VV("Create Output (Digital In) ID %#" B_PRIx32 "\n", mmc->id);

		nb_control_created++; mmc++;
	}

	//Internal mixer only for Output 1 and Digital Output
	if ((output == 0) || (output == 4)) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[22]);

		ITRACE_VV("Create Output (Mix); ID %#" B_PRIx32 "\n", mmc->id);

		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


uint32
ice1712UI_GetCombo(ice1712 *card, uint32 index)
{
	uint32 value = 0;

	switch (index) {
		case 0:
			value = card->settings.clock;
			break;

		case 1:
			value = card->settings.outFormat;
			break;

		case 2:
			value = card->settings.emphasis;
			break;

		case 3:
			value = card->settings.copyMode;
			break;
	}

	ITRACE_VV("Get combo: %" B_PRIu32 ", %" B_PRIu32 "\n",
		index, value);

	return value;
}


void
ice1712UI_SetCombo(ice1712 *card, uint32 index, uint32 value)
{
	ITRACE_VV("Set combo: %" B_PRIu32 ", %" B_PRIu32 "\n", index, value);

	switch (index) {
		case 0:
			if (value < 2)
				card->settings.clock = value;
			break;

		case 1:
			if (value < 2)
				card->settings.outFormat = value;
			break;

		case 2:
			if (value < 3)
				card->settings.emphasis = value;
			break;

		case 3:
			if (value < 3)
				card->settings.copyMode = value;
			break;
	}
}


uint32
ice1712UI_GetOutput(ice1712 *card, uint32 index)
{
	uint32 value = 0;

	if (index < 5)
		value = card->settings.output[index];

	ITRACE_VV("Get output: %" B_PRIu32 ", %" B_PRIu32 "\n", index, value);

	return value;
}


void
ice1712UI_SetOutput(ice1712 *card, uint32 index, uint32 value)
{
	if (index < 5)
		card->settings.output[index] = value;

	ITRACE_VV("Set output: %" B_PRIu32 ", %" B_PRIu32 "\n", index, value);
}


void
ice1712UI_GetVolume(ice1712 *card, multi_mix_value *mmv)
{
	ice1712Volume *vol;
	uint32 chan = ICE1712_MULTI_GET_CHANNEL(mmv->id);

	ITRACE_VV("Get volume\n");

	if (chan < ICE1712_HARDWARE_VOLUME) {
		vol = card->settings.playback;
	} else {
		vol = card->settings.record;
		chan -= ICE1712_HARDWARE_VOLUME;
	}

	//chan is normaly <= ICE1712_HARDWARE_VOLUME
	switch (ICE1712_MULTI_GET_INDEX(mmv->id)) {
		case 0: //Mute
			mmv->enable = vol[chan].mute | vol[chan + 1].mute;
			ITRACE_VV("	Get mute %d for channel %d or %d\n",
				mmv->enable, (int)chan, (int)chan + 1);
			break;

		case 2: //Right channel
			chan++;
			//No break
		case 1: //Left channel
			mmv->gain = vol[chan].volume;
			ITRACE_VV("	Get Volume %f for channel %d\n",
				mmv->gain, (int)chan);
			break;
	}
}


void
ice1712UI_SetVolume(ice1712 *card, multi_mix_value *mmv)
{
	ice1712Volume *vol;
	uint32 chan = ICE1712_MULTI_GET_CHANNEL(mmv->id);

	ITRACE_VV("Set volume\n");

	if (chan < ICE1712_HARDWARE_VOLUME) {
		vol = card->settings.playback;
	} else {
		vol = card->settings.record;
		chan -= ICE1712_HARDWARE_VOLUME;
	}

	//chan is normaly <= ICE1712_HARDWARE_VOLUME
	switch (ICE1712_MULTI_GET_INDEX(mmv->id)) {
		case 0: //Mute
			vol[chan].mute = mmv->enable;
			vol[chan + 1].mute = mmv->enable;
			ITRACE_VV("	Change mute to %d for channel %d and %d\n",
				mmv->enable, (int)chan, (int)chan + 1);
			break;

		case 2: //Right channel
			chan++;
			//No break
		case 1: //Left channel
			vol[chan].volume = mmv->gain;
			ITRACE_VV("	Change Volume to %f for channel %d\n",
				mmv->gain, (int)chan);
			break;
	}
}

//ICE1712 Multi - MIX
//-------------------

status_t
ice1712Get_MixValue(ice1712 *card, multi_mix_value_info *data)
{
	int i;

	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);
		ITRACE_VV("Get Mix: Id %" B_PRIu32 "\n", mmv->id);
		switch (mmv->id & ICE1712_MULTI_CONTROL_TYPE_MASK) {
			case ICE1712_MULTI_CONTROL_TYPE_COMBO:
				mmv->mux = ice1712UI_GetCombo(card,
					ICE1712_MULTI_GET_CHANNEL(mmv->id));
				break;

			case ICE1712_MULTI_CONTROL_TYPE_VOLUME:
				ice1712UI_GetVolume(card, mmv);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_OUTPUT:
				mmv->mux = ice1712UI_GetOutput(card,
					ICE1712_MULTI_GET_CHANNEL(mmv->id));
				break;

			default:
				ITRACE_VV("Get Mix: unknow %" B_PRIu32 "\n", mmv->id);
				break;
		}
	}

	return B_OK;
}


status_t
ice1712Set_MixValue(ice1712 *card, multi_mix_value_info *data)
{
	int i;

	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);
		ITRACE_VV("Set Mix: Id %" B_PRIu32 "\n", mmv->id);
		switch (mmv->id & ICE1712_MULTI_CONTROL_TYPE_MASK) {
			case ICE1712_MULTI_CONTROL_TYPE_COMBO:
				ice1712UI_SetCombo(card,
					ICE1712_MULTI_GET_CHANNEL(mmv->id), mmv->mux);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_VOLUME:
				ice1712UI_SetVolume(card, mmv);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_OUTPUT:
				ice1712UI_SetOutput(card,
					ICE1712_MULTI_GET_CHANNEL(mmv->id), mmv->mux);
				break;

			default:
				ITRACE_VV("Set Mix: unknow %" B_PRIu32 "\n", mmv->id);
				break;
		}
	}

	return ice1712Settings_apply(card);
}


/*
 * Not implemented
 */
status_t
ice1712Get_MixValueChannel(ice1712 *card, multi_mix_channel_info *data)
{
	return B_OK;
}


status_t
ice1712Get_MixValueControls(ice1712 *card, multi_mix_control_info *mmci)
{
	int32 i;
	uint32 parentTab, parentTabColumn;
	multi_mix_control *mmc = mmci->controls;
	uint32 group = 0, combo = 0, channel = 0;

	nb_control_created = 0;

	ITRACE_VV("Get MixValue Channels: Max %" B_PRIi32 "\n", mmci->control_count);

	//Cleaning
	memset(mmc, 0, mmci->control_count * sizeof(multi_mix_control));

	//Setup tab
	parentTab = ice1712UI_CreateGroup(&mmc, group++,
		CONTROL_IS_MASTER, S_SETUP, NULL);

	//General Settings
	parentTabColumn = ice1712UI_CreateGroup(&mmc, group++, parentTab,
		S_null, string_list[1]);
	for (i = 0; SettingsGeneral[i] != NULL; i++) {
		ice1712UI_CreateCombo(&mmc, SettingsGeneral[i], parentTabColumn,
			combo++, string_list[5 + i]);
	}

	//Digital Settings
	parentTabColumn = ice1712UI_CreateGroup(&mmc, group++, parentTab,
		S_null, string_list[2]);
	for (i = 0; SettingsDigital[i] != NULL; i++) {
		ice1712UI_CreateCombo(&mmc, SettingsDigital[i], parentTabColumn,
			combo++, string_list[8 + i]);
	}

	//Output Selection Settings
	parentTabColumn = ice1712UI_CreateGroup(&mmc, group++, parentTab,
		S_null, string_list[3]);
	for (i = 0; i < card->config.nb_DAC; i += 2) {
		ice1712UI_CreateOutput(card, &mmc, i / 2, parentTabColumn);
	}

	if (card->config.spdif & SPDIF_OUT_PRESENT) {
		ice1712UI_CreateOutput(card, &mmc, 4, parentTabColumn);
	}

	//Internal Mixer Tab
	//Output
	parentTab = ice1712UI_CreateGroup(&mmc, group++, CONTROL_IS_MASTER,
		S_null, string_list[4]);

	for (i = 0; i < card->config.nb_DAC; i += 2) {
		parentTabColumn = ice1712UI_CreateGroup(&mmc, group++,
			parentTab, S_null, string_list[(i / 2) + 11]);
		ice1712UI_CreateChannel(&mmc, channel++, parentTabColumn, NULL);
	}

	if (card->config.spdif & SPDIF_OUT_PRESENT) {
		parentTabColumn = ice1712UI_CreateGroup(&mmc, group++,
			parentTab, S_null, string_list[15]);
		ice1712UI_CreateChannel(&mmc, ICE1712_HARDWARE_VOLUME - 2,
			parentTabColumn, NULL);
	}

	//Input
	channel = ICE1712_HARDWARE_VOLUME;
	for (i = 0; i < card->config.nb_ADC; i += 2) {
		parentTabColumn = ice1712UI_CreateGroup(&mmc, group++,
			parentTab, S_null, string_list[(i / 2) + 17]);
		ice1712UI_CreateChannel(&mmc, channel++, parentTabColumn, NULL);
	}

	if (card->config.spdif & SPDIF_IN_PRESENT) {
		parentTabColumn = ice1712UI_CreateGroup(&mmc, group++,
			parentTab, S_null, string_list[21]);
		ice1712UI_CreateChannel(&mmc, 2 * ICE1712_HARDWARE_VOLUME - 2,
			parentTabColumn, NULL);
	}

	mmci->control_count = nb_control_created;
	ITRACE_VV("Get MixValue Channels: Returned %" B_PRIi32 "\n",
		mmci->control_count);

	return B_OK;
}


/*
 * Not implemented
 */
status_t
ice1712Get_MixValueConnections(ice1712 *card,
	multi_mix_connection_info *data)
{
	data->actual_count = 0;
	return B_OK;
}


status_t
ice1712Buffer_Get(ice1712 *card, multi_buffer_list *data)
{
	const size_t stride_o = MAX_DAC * SAMPLE_SIZE;
	const size_t stride_i = MAX_ADC * SAMPLE_SIZE;
	const uint32 buf_o = stride_o * card->buffer_size;
	const uint32 buf_i = stride_i * card->buffer_size;
	int buff, chan_i = 0, chan_o = 0;

	ITRACE_VV("flags = %#" B_PRIx32 "\n", data->flags);
	ITRACE_VV("request_playback_buffers = %" B_PRIu32 "\n",
			data->request_playback_buffers);
	ITRACE_VV("request_playback_channels = %" B_PRIu32 "\n",
			data->request_playback_channels);
	ITRACE_VV("request_playback_buffer_size = %" B_PRIx32 "\n",
			data->request_playback_buffer_size);
	ITRACE_VV("request_record_buffers = %" B_PRIu32 "\n",
			data->request_record_buffers);
	ITRACE_VV("request_record_channels = %" B_PRIu32 "\n",
			data->request_record_channels);
	ITRACE_VV("request_record_buffer_size = %" B_PRIx32 "\n",
			data->request_record_buffer_size);

	for (buff = 0; buff < SWAPPING_BUFFERS; buff++) {
		if (data->request_playback_channels == card->total_output_channels) {
			for (chan_o = 0; chan_o < card->config.nb_DAC; chan_o++) {
			//Analog STEREO output
				data->playback_buffers[buff][chan_o].base =
					(char*)(card->log_addr_pb + buf_o * buff
						+ SAMPLE_SIZE * chan_o);
				data->playback_buffers[buff][chan_o].stride = stride_o;
				ITRACE_VV("pb_buffer[%d][%d] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);
			}

			if (card->config.spdif & SPDIF_OUT_PRESENT) {
			//SPDIF STEREO output
				data->playback_buffers[buff][chan_o].base =
					(char*)(card->log_addr_pb + buf_o * buff
						+ SAMPLE_SIZE * SPDIF_LEFT);
				data->playback_buffers[buff][chan_o].stride = stride_o;
				ITRACE_VV("pb_buffer[%d][%d] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);

				chan_o++;
				data->playback_buffers[buff][chan_o].base =
					(char*)(card->log_addr_pb + buf_o * buff
						+ SAMPLE_SIZE * SPDIF_RIGHT);
				data->playback_buffers[buff][chan_o].stride = stride_o;
				ITRACE_VV("pb_buffer[%d][%d] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);
				chan_o++;
			}
		}

		if (data->request_record_channels ==
			card->total_input_channels) {
			for (chan_i = 0; chan_i < card->config.nb_ADC; chan_i++) {
			//Analog STEREO input
				data->record_buffers[buff][chan_i].base =
					(char*)(card->log_addr_rec + buf_i * buff
						+ SAMPLE_SIZE * chan_i);
				data->record_buffers[buff][chan_i].stride = stride_i;
				ITRACE_VV("rec_buffer[%d][%d] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
			}

			if (card->config.spdif & SPDIF_IN_PRESENT) {
			//SPDIF STEREO input
				data->record_buffers[buff][chan_i].base =
					(char*)(card->log_addr_rec + buf_i * buff
						+ SAMPLE_SIZE * SPDIF_LEFT);
				data->record_buffers[buff][chan_i].stride = stride_i;
				ITRACE_VV("rec_buffer[%d][%d] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);

				chan_i++;
				data->record_buffers[buff][chan_i].base =
					(char*)(card->log_addr_rec + buf_i * buff
						+ SAMPLE_SIZE * SPDIF_RIGHT);
				data->record_buffers[buff][chan_i].stride = stride_i;
				ITRACE_VV("rec_buffer[%d][%d] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
				chan_i++;
			}

			//The digital mixer output
			data->record_buffers[buff][chan_i].base =
				(char*)(card->log_addr_rec + buf_i * buff
					+ SAMPLE_SIZE * MIXER_OUT_LEFT);
			data->record_buffers[buff][chan_i].stride = stride_i;
			ITRACE_VV("rec_buffer[%d][%d] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);

			chan_i++;
			data->record_buffers[buff][chan_i].base =
				(char*)(card->log_addr_rec + buf_i * buff
					+ SAMPLE_SIZE * MIXER_OUT_RIGHT);
			data->record_buffers[buff][chan_i].stride = stride_i;
			ITRACE_VV("rec_buffer[%d][%d] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
			chan_i++;
		}
	}

	data->return_playback_buffers = SWAPPING_BUFFERS;
	data->return_playback_channels = card->total_output_channels;
	data->return_playback_buffer_size = card->buffer_size;

	ITRACE("return_playback_buffers = %" B_PRIi32 "\n",
		data->return_playback_buffers);
	ITRACE("return_playback_channels = %" B_PRIi32 "\n",
		data->return_playback_channels);
	ITRACE("return_playback_buffer_size = %" B_PRIu32 "\n",
		data->return_playback_buffer_size);

	data->return_record_buffers = SWAPPING_BUFFERS;
	data->return_record_channels = card->total_input_channels;
	data->return_record_channels = chan_i;
	data->return_record_buffer_size = card->buffer_size;

	ITRACE("return_record_buffers = %" B_PRIi32 "\n",
		data->return_record_buffers);
	ITRACE("return_record_channels = %" B_PRIi32 "\n",
		data->return_record_channels);
	ITRACE("return_record_buffer_size = %" B_PRIu32 "\n",
		data->return_record_buffer_size);

	ice1712Buffer_Start(card);

	return B_OK;
}
