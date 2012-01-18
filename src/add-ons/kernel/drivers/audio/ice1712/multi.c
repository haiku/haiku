/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */


#include "ice1712_reg.h"
#include "io.h"
#include "multi.h"
#include "util.h"

#include <string.h>
#include "debug.h"


#define AUTHORIZED_RATE (B_SR_SAME_AS_INPUT | B_SR_96000 \
	| B_SR_88200 | B_SR_48000 | B_SR_44100)
#define AUTHORIZED_SAMPLE_SIZE (B_FMT_32BIT)

#define MAX_CONTROL	32


static void
start_DMA(ice1712 *card)
{
	uint16 size = card->buffer_size * MAX_DAC;

	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

	write_mt_uint32(card, MT_PROF_PB_DMA_BASE_ADDRESS,
				 (uint32)card->phys_addr_pb);
	write_mt_uint16(card, MT_PROF_PB_DMA_COUNT_ADDRESS,
				 (size * SWAPPING_BUFFERS) - 1);
	//We want interrupt only from playback
	write_mt_uint16(card, MT_PROF_PB_DMA_TERM_COUNT, size - 1);
	TRACE("SIZE DMA PLAYBACK %#x\n", size);

	size = card->buffer_size * MAX_ADC;

	write_mt_uint32(card, MT_PROF_REC_DMA_BASE_ADDRESS,
				 (uint32)card->phys_addr_rec);
	write_mt_uint16(card, MT_PROF_REC_DMA_COUNT_ADDRESS,
				 (size * SWAPPING_BUFFERS) - 1);
	//We do not want any interrupt from the record
	write_mt_uint16(card, MT_PROF_REC_DMA_TERM_COUNT, 0);
	TRACE("SIZE DMA RECORD %#x\n", size);

	//Enable output AND Input from Analog CODEC
	switch (card->product) {
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
	switch (card->product) {
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
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_1);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_2);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_3);
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
ice1712_get_description(ice1712 *card, multi_description *data)
{
	int chan = 0, i, size;

	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	switch (card->product) {
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

	strncpy(data->vendor_info, "Haiku OS", 32);

	data->output_channel_count = card->total_output_channels;
	data->input_channel_count = card->total_input_channels;
	data->output_bus_channel_count = 0;
	data->input_bus_channel_count = 0;
	data->aux_bus_channel_count = 0;

	size =  data->output_channel_count + data->input_channel_count
		+ data->output_bus_channel_count + data->input_bus_channel_count
		+ data->aux_bus_channel_count;

	TRACE_VV("request_channel_count = %ld\n", data->request_channel_count);

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

	TRACE("output_channel_count = %ld\n", data->output_channel_count);
	TRACE("input_channel_count = %ld\n", data->input_channel_count);
	TRACE("output_bus_channel_count = %ld\n", data->output_bus_channel_count);
	TRACE("input_bus_channel_count = %ld\n", data->input_bus_channel_count);

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
ice1712_get_enabled_channels(ice1712 *card, multi_channel_enable *data)
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
ice1712_set_enabled_channels(ice1712 *card, multi_channel_enable *data)
{
	int i;
	int total_channel;

	total_channel = card->total_output_channels + card->total_input_channels;
	for (i = 0; i < total_channel; i++)
		TRACE("set_enabled_channels %d : %s\n", i,
			B_TEST_CHANNEL(data->enable_bits, i) ? "enabled": "disabled");

	TRACE("lock_source %#08X\n", (int)data->lock_source);
	TRACE("lock_data %#08X\n", (int)data->lock_data);

	if (data->lock_source == B_MULTI_LOCK_SPDIF)
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT, 0x10);
	else
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT, card->sampling_rate);

	card->lock_source = data->lock_source;

	return B_OK;
}


status_t
ice1712_get_global_format(ice1712 *card, multi_format_info *data)
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

	TRACE("Sampling Rate = %f\n", data->input.cvsr);

	return B_OK;
}


status_t
ice1712_set_global_format(ice1712 *card, multi_format_info *data)
{
	uint8 i;

	TRACE("Input Sampling Rate = %d\n", (int)data->input.rate);
	TRACE("Output Sampling Rate = %d\n", (int)data->output.rate);

	//We can't have a different rate for input and output
	//so just wait to change our sample rate when
	//media server will do what we need
	//Lie to it and say we are living in wonderland
	if (data->input.rate != data->output.rate)
		return B_OK;

	if (card->lock_source == B_MULTI_LOCK_INTERNAL) {
		switch (data->output.rate) {
			case B_SR_96000:
				card->sampling_rate = 0x07;
				break;
			case B_SR_88200:
				card->sampling_rate = 0x0B;
				break;
			case B_SR_48000:
				card->sampling_rate = 0x00;
				break;
			case B_SR_44100:
				card->sampling_rate = 0x08;
				break;
		}
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT, card->sampling_rate);
	}
	i = read_mt_uint8(card, MT_SAMPLING_RATE_SELECT);
	TRACE("New rate = %#x\n", i);

	return B_OK;
}


static uint32
get_combo_cb(ice1712 *card, uint32 index)
{
	uint32 value = 0;

	TRACE_VV("   get_combo_cb: %ld, %ld\n", index, value);

	switch (index) {
		case 0:
			value = card->settings.clock;
			break;

		case 1:
			value = card->settings.bufferSize;
			break;

		case 2:
			value = card->settings.outFormat;
			break;

		case 3:
			value = card->settings.emphasis;
			break;

		case 4:
			value = card->settings.copyMode;
			break;
	}

	return value;
}


static void
set_combo_cb(ice1712 *card, uint32 index, uint32 value)
{
	TRACE_VV("   set_combo_cb: %ld, %ld\n", index, value);

	switch (index) {
		case 0:
			if (value < 2)
				card->settings.clock = value;
			break;

		case 1:
			if (value < 6) {
				card->settings.bufferSize = value;
//				card->buffer_size = 64 * (1 << value);
//				ice1712_buffer_force_stop(card);
//				start_DMA(card);
			}
			break;

		case 2:
			if (value < 2)
				card->settings.outFormat = value;
			break;

		case 3:
			if (value < 3)
				card->settings.emphasis = value;
			break;

		case 4:
			if (value < 3)
				card->settings.copyMode = value;
			break;
	}
}


static uint32
get_output_cb(ice1712 *card, uint32 index)
{
	uint32 value = 0;

	if (index < 5)
		value = card->settings.output[index];

	TRACE_VV("   get_output_cb: %ld, %ld\n", index, value);

	return value;
}


static void
set_output_cb(ice1712 *card, uint32 index, uint32 value)
{
	if (index < 5)
		card->settings.output[index] = value;

	TRACE_VV("   set_output_cb: %ld, %ld\n", index, value);
}


static void
get_volume_cb(ice1712 *card, multi_mix_value *mmv)
{
	channel_volume *vol;
	uint32 chan = ICE1712_MULTI_GET_CHANNEL(mmv->id);

	TRACE_VV("   get_volume_cb\n");

	if (chan < ICE1712_HARDWARE_VOLUME) {
		vol = card->settings.playback;
	}
	else {
		vol = card->settings.record;
		chan -= ICE1712_HARDWARE_VOLUME;
	}

	//chan is normaly <= ICE1712_HARDWARE_VOLUME
	switch (ICE1712_MULTI_GET_INDEX(mmv->id)) {
		case 0: //Mute
			mmv->u.enable = vol[chan].mute | vol[chan + 1].mute;
			TRACE_VV("\tGet mute %d for channel %d or %d\n",
				mmv->u.enable, (int)chan, (int)chan + 1);
			break;

		case 2: //Right channel
			chan++;
			//No break
		case 1: //Left channel
			mmv->u.gain = vol[chan].volume;
			TRACE_VV("\tGet Volume %f for channel %d\n",
				mmv->u.gain, (int)chan);
			break;
	}
}


static void
set_volume_cb(ice1712 *card, multi_mix_value *mmv)
{
	channel_volume *vol;
	uint32 chan = ICE1712_MULTI_GET_CHANNEL(mmv->id);

	TRACE_VV("   set_volume_cb\n");

	if (chan < ICE1712_HARDWARE_VOLUME) {
		vol = card->settings.playback;
	}
	else {
		vol = card->settings.record;
		chan -= ICE1712_HARDWARE_VOLUME;
	}

	//chan is normaly <= ICE1712_HARDWARE_VOLUME
	switch (ICE1712_MULTI_GET_INDEX(mmv->id)) {
		case 0: //Mute
			vol[chan].mute = mmv->u.enable;
			vol[chan + 1].mute = mmv->u.enable;
			TRACE_VV("\tChange mute to %d for channel %d and %d\n",
				mmv->u.enable, (int)chan, (int)chan + 1);
			break;

		case 2: //Right channel
			chan++;
			//No break
		case 1: //Left channel
			vol[chan].volume = mmv->u.gain;
			TRACE_VV("\tChange Volume to %f for channel %d\n",
				mmv->u.gain, (int)chan);
			break;
	}
}


status_t
ice1712_get_mix(ice1712 *card, multi_mix_value_info *data)
{
	int i;
	TRACE_VV("	Asking to get %ld control(s)\n", data->item_count);

	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);
		TRACE_VV("	 Id %#x\n", (unsigned int)mmv->id);
		switch (mmv->id & ICE1712_MULTI_CONTROL_TYPE_MASK) {
			case ICE1712_MULTI_CONTROL_TYPE_COMBO:
				mmv->u.mux = get_combo_cb(card,
								ICE1712_MULTI_GET_CHANNEL(mmv->id));
				break;

			case ICE1712_MULTI_CONTROL_TYPE_VOLUME:
				get_volume_cb(card, mmv);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_OUTPUT:
				mmv->u.mux = get_output_cb(card,
								ICE1712_MULTI_GET_CHANNEL(mmv->id));
				break;

			default:
				TRACE_VV("	  default 0x%x\n", (unsigned int)mmv->id);
				break;
		}
	}

	return B_OK;
}


status_t
ice1712_set_mix(ice1712 *card, multi_mix_value_info *data)
{
	int i;

	TRACE_VV("	Asking to set %ld control(s)\n", data->item_count);

	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);
		TRACE_VV("	 Id %#x\n", (unsigned int)mmv->id);
		switch (mmv->id & ICE1712_MULTI_CONTROL_TYPE_MASK) {
			case ICE1712_MULTI_CONTROL_TYPE_COMBO:
				set_combo_cb(card, ICE1712_MULTI_GET_CHANNEL(mmv->id),
					mmv->u.mux);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_VOLUME:
				set_volume_cb(card, mmv);
				break;

			case ICE1712_MULTI_CONTROL_TYPE_OUTPUT:
				set_output_cb(card, ICE1712_MULTI_GET_CHANNEL(mmv->id),
					mmv->u.mux);
				break;

			default:
				TRACE_VV("	  default 0x%x\n", (unsigned int)mmv->id);
				break;
		}
	}

	return applySettings(card);
}


status_t
ice1712_list_mix_channels(ice1712 *card, multi_mix_channel_info *data)
{
	//Not Implemented
	return B_OK;
}

static const char *Clock[] = {
	"Internal",
	"Digital",
	NULL,
};

static const char *BufferSize[] = {
	"64",
	"128",
	"256",
	"512",
	"1024",
	"2048",
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
	BufferSize,
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
	"Buffer size",
	"Debug",

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

static int32 nb_control_created;


//This will create a Tab
static int32
create_group_control(multi_mix_control **p_mmc, int32 index, int32 parent,
	enum strind_id string, const char* name)
{
	multi_mix_control *mmc = *p_mmc;
	int32 group;

	TRACE_VV("Create ID create_group_control\n");

	mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + ICE1712_MULTI_SET_INDEX(index);
	mmc->parent = parent;
	mmc->flags = B_MULTI_MIX_GROUP;
	mmc->master = CONTROL_IS_MASTER;
	mmc->string = string;

	group = mmc->id;

	if (name != NULL)
		strcpy(mmc->name, name);

	TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

	nb_control_created++; mmc++;
	(*p_mmc) = mmc;

	return group;
}


//This will create a Slider with a "Mute" CheckBox
static void
create_channel_control(multi_mix_control **p_mmc, int32 channel, int32 parent,
	const char* name)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_VOLUME
		+ ICE1712_MULTI_SET_CHANNEL(channel);
	multi_mix_control *mmc = *p_mmc;
	multi_mix_control control;

	TRACE_VV("Create ID create_channel_control\n");

	control.master = CONTROL_IS_MASTER;
	control.parent = parent;
	control.u.gain.max_gain = 0.0;
	control.u.gain.min_gain = -144.0;
	control.u.gain.granularity = 1.5;

	//The Mute Checkbox
	control.id = id++;
	control.flags = B_MULTI_MIX_ENABLE;
	control.string = S_MUTE;
	*mmc = control;
	mmc++;

	TRACE_VV("Create ID %#x\n", (unsigned int)control.id);

	//The Left Slider
	control.string = S_null;
	control.id = id++;
	control.flags = B_MULTI_MIX_GAIN;
	if (name != NULL)
		strcpy(control.name, name);
	*mmc = control;
	mmc++;

	TRACE_VV("Create ID %#x\n", (unsigned int)control.id);

	//The Right Slider
	control.master = control.id; //The Id of the Left Slider
	control.id = id++;
	*mmc = control;
	mmc++;

	TRACE_VV("Create ID %#x\n", (unsigned int)control.id);

	nb_control_created += 3;
	(*p_mmc) = mmc;
}


static void
create_combo_control(multi_mix_control **p_mmc, const char *values[],
	int32 parent, int32 nb_combo, const char *name)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_COMBO
		+ ICE1712_MULTI_SET_CHANNEL(nb_combo);
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	TRACE_VV("Create ID create_combo_control\n");

	//The label
	parentControl = mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, name);

	TRACE_VV("Create ID %#x\n", (unsigned int)parentControl);

	nb_control_created++; mmc++;

	//The values
	for (i = 0; values[i] != NULL; i++) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, values[i]);

		TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


//This will create all possible value for the output
//output 0 -> 3 (physical stereo output) 4 is the Digital
static void
create_output_choice(ice1712 *card, multi_mix_control **p_mmc,
	int32 output, int32 parent)
{
	int32 id = ICE1712_MULTI_CONTROL_FIRSTID
		+ ICE1712_MULTI_CONTROL_TYPE_OUTPUT
		+ ICE1712_MULTI_SET_CHANNEL(output);
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	TRACE_VV("Create ID create_output_choice\n");

	//The label
	parentControl = mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, string_list[11 + output]);
	nb_control_created++; mmc++;

	TRACE_VV("Create ID %#x\n", (unsigned int)parentControl);

	//Haiku output
	mmc->id = id++;
	mmc->flags = B_MULTI_MIX_MUX_VALUE;
	mmc->parent = parentControl;
	strcpy(mmc->name, string_list[16]);

	TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

	nb_control_created++; mmc++;

	//Physical Input
	for (i = 0; i < card->config.nb_DAC; i += 2) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[17 + (i / 2)]);

		TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

		nb_control_created++; mmc++;
	}

	//Physical Digital Input
	if (card->config.spdif & SPDIF_IN_PRESENT) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[21]);

		TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

		nb_control_created++; mmc++;
	}

	//Internal mixer only for Output 1 and Digital Output
	if ((output == 0) || (output == 4)) {
		mmc->id = id++;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[22]);

		TRACE_VV("Create ID %#x\n", (unsigned int)mmc->id);

		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


status_t
ice1712_list_mix_controls(ice1712 *card, multi_mix_control_info *mmci)
{
	uint32 i, parentTab, parentTabColumn;
	multi_mix_control *mmc = mmci->controls;
	uint32 group = 0, combo = 0, channel = 0;

	nb_control_created = 0;

	TRACE_VV("count = %ld\n", mmci->control_count);

	//Cleaning
	memset(mmc, 0, mmci->control_count * sizeof(multi_mix_control));

	//Setup tab
	parentTab = create_group_control(&mmc, group++,
		CONTROL_IS_MASTER, S_SETUP, NULL);

	//General Settings
	parentTabColumn = create_group_control(&mmc, group++, parentTab,
		S_null, string_list[1]);
	for (i = 0; SettingsGeneral[i] != NULL; i++) {
		create_combo_control(&mmc, SettingsGeneral[i], parentTabColumn,
			combo++, string_list[5 + i]);
	}

	//Digital Settings
	parentTabColumn = create_group_control(&mmc, group++, parentTab,
		S_null, string_list[2]);
	for (i = 0; SettingsDigital[i] != NULL; i++) {
		create_combo_control(&mmc, SettingsDigital[i], parentTabColumn,
			combo++, string_list[8 + i]);
	}

	//Output Selection Settings
	parentTabColumn = create_group_control(&mmc, group++, parentTab,
		S_null, string_list[3]);
	for (i = 0; i < card->config.nb_DAC; i += 2) {
		create_output_choice(card, &mmc, i / 2, parentTabColumn);
	}

	if (card->config.spdif & SPDIF_OUT_PRESENT) {
		create_output_choice(card, &mmc, 4, parentTabColumn);
	}

	//Internal Mixer Tab
	//Output
	parentTab = create_group_control(&mmc, group++, CONTROL_IS_MASTER,
		S_null, string_list[4]);

	for (i = 0; i < card->config.nb_DAC; i += 2) {
		parentTabColumn = create_group_control(&mmc, group++, parentTab,
			S_null, string_list[(i / 2) + 11]);
		create_channel_control(&mmc, channel++, parentTabColumn, NULL);
	}

	if (card->config.spdif & SPDIF_OUT_PRESENT) {
		parentTabColumn = create_group_control(&mmc, group++, parentTab,
			S_null, string_list[15]);
		create_channel_control(&mmc, ICE1712_HARDWARE_VOLUME - 2,
			parentTabColumn, NULL);
	}

	//Input
	channel = ICE1712_HARDWARE_VOLUME;
	for (i = 0; i < card->config.nb_ADC; i += 2) {
		parentTabColumn = create_group_control(&mmc, group++, parentTab,
			 S_null, string_list[(i / 2) + 17]);
		create_channel_control(&mmc, channel++, parentTabColumn, NULL);
	}

	if (card->config.spdif & SPDIF_IN_PRESENT) {
		parentTabColumn = create_group_control(&mmc, group++, parentTab,
			S_null, string_list[21]);
		create_channel_control(&mmc, 2 * ICE1712_HARDWARE_VOLUME - 2,
			parentTabColumn, NULL);
	}

	TRACE_VV("Return %ld control(s)\n", nb_control_created);
	mmci->control_count = nb_control_created;

	return B_OK;
}


status_t
ice1712_list_mix_connections(ice1712 *card, multi_mix_connection_info *data)
{//Not Implemented
	data->actual_count = 0;
	return B_OK;
}


status_t
ice1712_get_buffers(ice1712 *card, multi_buffer_list *data)
{
	int buff, chan_i = 0, chan_o = 0;

	TRACE_VV("flags = %#lx\n", data->flags);
	TRACE_VV("request_playback_buffers = %ld\n",
		  data->request_playback_buffers);
	TRACE_VV("request_playback_channels = %ld\n",
		  data->request_playback_channels);
	TRACE_VV("request_playback_buffer_size = %lx\n",
		  data->request_playback_buffer_size);
	TRACE_VV("request_record_buffers = %ld\n",
		  data->request_record_buffers);
	TRACE_VV("request_record_channels = %ld\n",
		  data->request_record_channels);
	TRACE_VV("request_record_buffer_size = %lx\n",
		  data->request_record_buffer_size);

	for (buff = 0; buff < SWAPPING_BUFFERS; buff++) {
		uint32 stride_o = MAX_DAC * SAMPLE_SIZE;
		uint32 stride_i = MAX_ADC * SAMPLE_SIZE;
		uint32 buf_o = stride_o * card->buffer_size;
		uint32 buf_i = stride_i * card->buffer_size;

		if (data->request_playback_channels == card->total_output_channels) {
			for (chan_o = 0; chan_o < card->config.nb_DAC; chan_o++) {
			//Analog STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb
					+ buf_o * buff + SAMPLE_SIZE * chan_o;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_VV("pb_buffer[%ld][%ld] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);
			}

			if (card->config.spdif & SPDIF_OUT_PRESENT) {
			//SPDIF STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb
					+ buf_o * buff + SAMPLE_SIZE * SPDIF_LEFT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_VV("pb_buffer[%ld][%ld] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);

				chan_o++;
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb
					+ buf_o * buff + SAMPLE_SIZE * SPDIF_RIGHT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_VV("pb_buffer[%ld][%ld] = %p\n", buff, chan_o,
					data->playback_buffers[buff][chan_o].base);
				chan_o++;
			}
		}

		if (data->request_record_channels == card->total_input_channels) {
			for (chan_i = 0; chan_i < card->config.nb_ADC; chan_i++) {
			//Analog STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * chan_i;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_VV("rec_buffer[%ld][%ld] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
			}

			if (card->config.spdif & SPDIF_IN_PRESENT) {
			//SPDIF STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * SPDIF_LEFT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_VV("rec_buffer[%ld][%ld] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);

				chan_i++;
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * SPDIF_RIGHT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_VV("rec_buffer[%ld][%ld] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
				chan_i++;
			}

			//The digital mixer output
			data->record_buffers[buff][chan_i].base = card->log_addr_rec
				+ buf_i * buff + SAMPLE_SIZE * MIXER_OUT_LEFT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			TRACE_VV("rec_buffer[%ld][%ld] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);

			chan_i++;
			data->record_buffers[buff][chan_i].base = card->log_addr_rec
				+ buf_i * buff + SAMPLE_SIZE * MIXER_OUT_RIGHT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			TRACE_VV("rec_buffer[%ld][%ld] = %p\n", buff, chan_i,
					data->record_buffers[buff][chan_i].base);
			chan_i++;
		}
	}

	data->return_playback_buffers = SWAPPING_BUFFERS;
	data->return_playback_channels = card->total_output_channels;
	data->return_playback_buffer_size = card->buffer_size;

	TRACE("return_playback_buffers = %ld\n", data->return_playback_buffers);
	TRACE("return_playback_channels = %ld\n", data->return_playback_channels);
	TRACE("return_playback_buffer_size = %ld\n",
		data->return_playback_buffer_size);

	data->return_record_buffers = SWAPPING_BUFFERS;
	data->return_record_channels = card->total_input_channels;
	data->return_record_channels = chan_i;
	data->return_record_buffer_size = card->buffer_size;

	TRACE("return_record_buffers = %ld\n", data->return_record_buffers);
	TRACE("return_record_channels = %ld\n", data->return_record_channels);
	TRACE("return_record_buffer_size = %ld\n",
		data->return_record_buffer_size);

	start_DMA(card);

	return B_OK;
}


status_t
ice1712_buffer_exchange(ice1712 *card, multi_buffer_info *data)
{
	int cpu_status;

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
		TRACE("buffer_exchange timeout\n");
	};

	cpu_status = lock();

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

	unlock(cpu_status);

/*	if ((card->buffer % 1500) == 0) {
		uint8 reg8, reg8_dir;
		reg8 = read_gpio(card);
		reg8_dir = read_cci_uint8(card, CCI_GPIO_DIRECTION_CONTROL);
		TRACE("DEBUG === GPIO = %d (dir %d)\n", reg8, reg8_dir);

		reg8 = spdif_read(card, CS84xx_VERSION_AND_CHIP_ID);
		TRACE("DEBUG === S/PDif Version : 0x%x\n", reg8);
	}*/

#ifdef __HAIKU__
	if (user_memcpy(data, &buffer_info, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(data, &buffer_info, sizeof(buffer_info));
#endif

	return B_OK;
}

status_t ice1712_buffer_force_stop(ice1712 *card)
{
//	int cpu_status;

	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

//	cpu_status = lock();

	card->played_time = 0;
	card->frames_count = 0;
	card->buffer = 0;

//	unlock(cpu_status);

	return B_OK;
}

