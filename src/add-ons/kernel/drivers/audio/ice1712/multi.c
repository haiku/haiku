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

#include "debug.h"
#include "ice1712_reg.h"
#include "io.h"
#include "multi.h"
#include "util.h"

#include <string.h>

#define AUTHORIZED_RATE (B_SR_SAME_AS_INPUT | B_SR_IS_GLOBAL | B_SR_96000 \
	| B_SR_88200 | B_SR_48000 | B_SR_44100)
#define AUTHORIZED_SAMPLE_SIZE (B_FMT_32BIT)


static void 
start_DMA(ice1712 *card)
{
	uint16 size = card->buffer_size * MAX_DAC;

	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

	write_mt_uint32(card, MT_PROF_PB_DMA_BASE_ADDRESS, (uint32)card->phys_addr_pb);
	write_mt_uint16(card, MT_PROF_PB_DMA_COUNT_ADDRESS, (size * SWAPPING_BUFFERS) - 1);
	//We want interrupt only from playback
	write_mt_uint16(card, MT_PROF_PB_DMA_TERM_COUNT, size - 1);
	TRACE("SIZE DMA PLAYBACK %#x\n", size);

	size = card->buffer_size * MAX_ADC;

	write_mt_uint32(card, MT_PROF_REC_DMA_BASE_ADDRESS,	(uint32)card->phys_addr_rec);
	write_mt_uint16(card, MT_PROF_REC_DMA_COUNT_ADDRESS, (size * SWAPPING_BUFFERS) - 1);
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
	int chan = 0, i;

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
			
	strncpy(data->vendor_info, "Jerome Leveque", 32);
	
	data->output_channel_count = card->total_output_channels;
	data->input_channel_count = card->total_input_channels;
	data->output_bus_channel_count = 0;
	data->input_bus_channel_count = 0;
	data->aux_bus_channel_count = 0;

	if (ICE1712_VERY_VERBOSE)
		TRACE("request_channel_count = %ld\n", data->request_channel_count);

	if (data->request_channel_count >= (card->total_output_channels 
		+ card->total_input_channels)) {
		for (i = 0; i < card->nb_DAC; i++) {
		//Analog STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| ((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT;
			data->channels[chan++].connectors = B_CHANNEL_RCA;
		}

		if (card->spdif_config & NO_IN_YES_OUT) {
		//SPDIF STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS 
				| B_CHANNEL_LEFT;
			data->channels[chan].connectors = B_CHANNEL_COAX_SPDIF;
			chan++;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS
				| B_CHANNEL_RIGHT;
			data->channels[chan++].connectors = B_CHANNEL_COAX_SPDIF;
		}

		for (i = 0; i < card->nb_ADC; i++) {
		//Analog STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS |
									((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT;
			data->channels[chan++].connectors = B_CHANNEL_RCA;
		}

		if (card->spdif_config & YES_IN_NO_OUT) {
		//SPDIF STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_LEFT;
			data->channels[chan++].connectors = B_CHANNEL_COAX_SPDIF;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_BUS;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_RIGHT;
			data->channels[chan++].connectors = B_CHANNEL_COAX_SPDIF;
		}

		//The digital mixer output
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_BUS;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_LEFT;
		data->channels[chan++].connectors = B_CHANNEL_SPDIF_HEADER;
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_BUS;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_RIGHT;
		data->channels[chan++].connectors = B_CHANNEL_SPDIF_HEADER;
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
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	data->start_latency = 0;

	strcpy(data->control_panel,"");

	return B_OK;
}


status_t 
ice1712_get_enabled_channels(ice1712 *card, multi_channel_enable *data)
{
	int i;
	uint8 reg;
	for (i = 0; i < (card->total_output_channels + card->total_input_channels); i++)
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
	for (i = 0; i < (card->total_output_channels + card->total_input_channels); i++)
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
		case 0x0:
			data->input.rate = data->output.rate = B_SR_48000;
			data->input.cvsr = data->output.cvsr = 48000.0f;
			break;
		case 0x7:
			data->input.rate = data->output.rate = B_SR_96000;
			data->input.cvsr = data->output.cvsr = 96000.0f;
			break;
		case 0x8:
			data->input.rate = data->output.rate = B_SR_44100;
			data->input.cvsr = data->output.cvsr = 44100.0f;
			break;
		case 0xb:
			data->input.rate = data->output.rate = B_SR_88200;
			data->input.cvsr = data->output.cvsr = 88200.0f;
			break;
	}
	
	data->timecode_kind = 0;
	data->output_latency = data->input_latency = 0;
	data->output.format = data->input.format = AUTHORIZED_SAMPLE_SIZE;

	TRACE("Input Sampling Rate = %d\n", (int)data->input.cvsr);
	TRACE("Output Sampling Rate = %d\n", (int)data->output.cvsr);

	return B_OK;
}


status_t 
ice1712_set_global_format(ice1712 *card, multi_format_info *data)
{
	uint8 i;

	TRACE("Input Sampling Rate = %#x\n", (int)data->input.rate);
	TRACE("Output Sampling Rate = %#x\n", (int)data->output.rate);

	if ((data->input.rate == data->output.rate)
		&& (card->lock_source == B_MULTI_LOCK_INTERNAL)) {
		switch (data->input.rate) {
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
	TRACE("RATE = %#x\n", i);
	return B_OK;
}


status_t 
ice1712_get_mix(ice1712 *card, multi_mix_value_info *data)
{
	int i;
	TRACE("	Asking to get %ld control(s)\n", data->item_count);

	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);

		switch (mmv->id & ICE1712_MULTI_CONTROL_MASK) {
			case ICE1712_MULTI_CONTROL_VOLUME_PB:
				mmv->u.gain = card->settings.playback[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].volume;
				break;

			case ICE1712_MULTI_CONTROL_VOLUME_REC:
				mmv->u.gain = card->settings.record[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].volume;
				break;

			case ICE1712_MULTI_CONTROL_MUTE | ICE1712_MULTI_CONTROL_VOLUME_PB:
				mmv->u.enable = card->settings.playback[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].mute;
				break;

			case ICE1712_MULTI_CONTROL_MUTE | ICE1712_MULTI_CONTROL_VOLUME_REC:
				mmv->u.enable = card->settings.record[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].mute;
				break;

			case ICE1712_MULTI_CONTROL_MUX:
				mmv->u.mux = 0;
				break;
		}
	}

	return B_OK;
}


status_t 
ice1712_set_mix(ice1712 *card, multi_mix_value_info *data)
{
	int i;
	TRACE("	Asking to set %ld control(s)\n", data->item_count);
	
	for (i = 0; i < data->item_count; i++) {
		multi_mix_value *mmv = &(data->values[i]);

		switch (mmv->id & ICE1712_MULTI_CONTROL_MASK) {
			case ICE1712_MULTI_CONTROL_VOLUME_PB:
				card->settings.playback[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].volume = mmv->u.gain;
				TRACE("	ICE1712_MULTI_CONTROL_VOLUME_PB 0x%x : %f\n", (unsigned int)mmv->id, mmv->u.gain);
				break;

			case ICE1712_MULTI_CONTROL_VOLUME_REC:
				card->settings.record[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].volume = mmv->u.gain;
				TRACE("	ICE1712_MULTI_CONTROL_VOLUME_REC 0x%x : %f\n", (unsigned int)mmv->id, mmv->u.gain);
				break;

			case ICE1712_MULTI_CONTROL_MUTE | ICE1712_MULTI_CONTROL_VOLUME_PB:
				card->settings.playback[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].mute = mmv->u.enable;
				card->settings.playback[(mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK) + 1].mute = mmv->u.enable;
				TRACE("	ICE1712_MULTI_CONTROL_MUTE_PB 0x%x : %d\n", (unsigned int)mmv->id, mmv->u.enable);
				break;

			case ICE1712_MULTI_CONTROL_MUTE | ICE1712_MULTI_CONTROL_VOLUME_REC:
				card->settings.record[mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK].mute = mmv->u.enable;
				card->settings.record[(mmv->id & ICE1712_MULTI_CONTROL_CHANNEL_MASK) + 1].mute = mmv->u.enable;
				TRACE("	ICE1712_MULTI_CONTROL_MUTE_REC 0x%x : %d\n", (unsigned int)mmv->id, mmv->u.enable);
				break;

			case ICE1712_MULTI_CONTROL_MUX:
				TRACE("	ICE1712_MULTI_CONTROL_MUX 0x%x : 0x%x\n", (unsigned int)mmv->id, (unsigned int)mmv->u.mux);
				break;
				
			default:
				TRACE("	default 0x%x\n", (unsigned int)mmv->id);
				break;
		}
	}

	return apply_settings(card);
}


status_t 
ice1712_list_mix_channels(ice1712 *card, multi_mix_channel_info *data)
{//Not Implemented
	return B_OK;
}

static const char *Clock[] = { "Internal", "Digital", NULL };
static const char *SampleRate[] = { "44,1 KHz", "48 KHz", "88,2 KHz",
	"96 KHz", NULL };
static const char *BufferSize[] = { "64", "128", "256", "512", "1024",
	"2048", NULL };
#if 0
static const char *Debug[] = { "Mode 0", "Mode 1", "Mode 2", "Mode 3", 
	"Mode 4", "Mode 5", NULL };
#endif
static const char *DigitalFormat[] = { "Consumer", "Professional", NULL };
static const char *DigitalEmphasis[] = { "None", "CCITT", "15/50usec", NULL };
static const char *DigitalCopyMode[] = { "Original", "1st Generation",
	"No SCMS", NULL };


static const char **SettingsGeneral[] = { Clock, SampleRate, BufferSize,
#if 0
	Debug,
#endif
	NULL };

static const char **SettingsDigital[] = { DigitalFormat, DigitalEmphasis, 
	DigitalCopyMode, NULL };
static const char *string_list[] = { "Setup", "General Not implemented yet",
	"Digital Not implemented yet", "Output Selection Not implemented yet",
	"Internal Mixer", 
	
	//General settings
	"Master clock", "Sample rate", "Buffer size", "Debug",

	//Digital settings
	"Output format", "Emphasis", "Copy mode",

	//Output Selection
	"Output 1", //12
	"Output 2", 
	"Output 3",
	"Output 4",
	"Digital Output", //16

	"Haiku output", //17

	"Input 1", //18
	"Input 2",
	"Input 3",
	"Input 4",
	"Digital Input", //22
	"Internal mixer", //23
};

static int32 nb_control_created;


//This will create a Tab
static int32
create_group_control(multi_mix_control **p_mmc, int32 offset, int32 parent,
	enum strind_id string, const char* name)
{
	multi_mix_control *mmc = *p_mmc;
	int32 group;
	//Increase the index cause we fill it

	group = mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + offset;
	mmc->parent = parent;
	mmc->flags = B_MULTI_MIX_GROUP;
	mmc->master = CONTROL_IS_MASTER;
	mmc->string = string;

	if (name != NULL)
		strcpy(mmc->name, name);

	nb_control_created++; mmc++;
	(*p_mmc) = mmc;

	return group;
}


//This will create a Slider with a "Mute" CheckBox
static void
create_channel_control(multi_mix_control **p_mmc, int32 offset, int32 parent,
	const char* name)
{
	multi_mix_control *mmc = *p_mmc;
	multi_mix_control control;
	
	control.master = CONTROL_IS_MASTER;
	control.parent = parent;
	control.u.gain.max_gain = 0.0;
	control.u.gain.min_gain = -144.0;
	control.u.gain.granularity = 1.5;

	//The Mute Checkbox
	control.id = ICE1712_MULTI_CONTROL_FIRSTID + ICE1712_MULTI_CONTROL_MUTE + offset;
	control.flags = B_MULTI_MIX_ENABLE;
	control.string = S_MUTE;
	*mmc = control;
	mmc++;

	//The Left Slider
	control.string = S_null;
	control.id = ICE1712_MULTI_CONTROL_FIRSTID + offset;
	control.flags = B_MULTI_MIX_GAIN;
	if (name != NULL)
		strcpy(control.name, name);
	*mmc = control;
	mmc++;

	//The Right Slider
	control.master = control.id; //The Id of the Left Slider
	control.id++;
	*mmc = control;
	mmc++;

	nb_control_created += 3;
	(*p_mmc) = mmc;
}


static void
create_combo_control(multi_mix_control **p_mmc, const char *values[], 
	int32 parent, const char *name)
{
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	//The label
	parentControl = mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + ICE1712_MULTI_CONTROL_MUX + nb_control_created;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, name);
	nb_control_created++; mmc++;

	//The values
	for (i = 0; values[i] != NULL; i++) {
		mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + nb_control_created;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, values[i]);
		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


//This will create all possible value for the output
//output 0 -> 3 (physical stereo output) 4 is the Digital
static void
create_output_choice(ice1712 *card, multi_mix_control **p_mmc, int32 output, int32 parent)
{
	multi_mix_control *mmc = *p_mmc;
	int32 parentControl, i;

	//The label
	parentControl = mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + ICE1712_MULTI_CONTROL_MUX + nb_control_created;
	mmc->flags = B_MULTI_MIX_MUX;
	mmc->parent = parent;
	strcpy(mmc->name, string_list[12 + output]);
	nb_control_created++; mmc++;

	//Haiku output
	mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + nb_control_created;
	mmc->flags = B_MULTI_MIX_MUX_VALUE;
	mmc->parent = parentControl;
	strcpy(mmc->name, string_list[17]);
	nb_control_created++; mmc++;

	//Physical Input
	for (i = 0; i < card->nb_DAC; i += 2) {
		mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + nb_control_created;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[18 + (i / 2)]);
		nb_control_created++; mmc++;
	}
	
	//Physical Digital Input
	if (card->spdif_config & YES_IN_NO_OUT) {
		mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + nb_control_created;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[22]);
		nb_control_created++; mmc++;
	}

	//Internal mixer only for Output 1 andd Digital Output
	if ((output == 0) || (output == 4)) {
		mmc->id = ICE1712_MULTI_CONTROL_FIRSTID + nb_control_created;
		mmc->flags = B_MULTI_MIX_MUX_VALUE;
		mmc->parent = parentControl;
		strcpy(mmc->name, string_list[23]);
		nb_control_created++; mmc++;
	}

	(*p_mmc) = mmc;
}


status_t
ice1712_list_mix_controls(ice1712 *card, multi_mix_control_info *mmci)
{
	int32 i, parentTab, parentTabColumn;
	multi_mix_control *mmc = mmci->controls;

	nb_control_created = 0;

	TRACE("count = %ld\n", mmci->control_count);

	//Cleaning
	memset(mmc, 0, mmci->control_count * sizeof(multi_mix_control));

	//Setup tab
	parentTab = create_group_control(&mmc, nb_control_created, CONTROL_IS_MASTER, S_SETUP, NULL);

	//General Settings
	parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[1]);
	for (i = 0; SettingsGeneral[i] != NULL; i++)
		create_combo_control(&mmc, SettingsGeneral[i], parentTabColumn, string_list[5 + i]);

	//Digital Settings
	parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[2]);
	for (i = 0; SettingsDigital[i] != NULL; i++)
		create_combo_control(&mmc, SettingsDigital[i], parentTabColumn, string_list[9 + i]);

	//Output Selection Settings
	parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[3]);
	for (i = 0; i < card->nb_DAC; i += 2)
		create_output_choice(card, &mmc, i / 2, parentTabColumn);

	if (card->spdif_config & NO_IN_YES_OUT)
		create_output_choice(card, &mmc, 4, parentTabColumn);

	//Internal Mixer Tab
	parentTab = create_group_control(&mmc, nb_control_created, CONTROL_IS_MASTER, S_null, string_list[4]);

	for (i = 0; i < card->nb_DAC; i += 2) {
		parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[(i / 2) + 18]);
		create_channel_control(&mmc, ICE1712_MULTI_CONTROL_VOLUME_PB + i, parentTabColumn, NULL);
	}

	if (card->spdif_config & YES_IN_NO_OUT) {
		parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[22]);
		create_channel_control(&mmc, ICE1712_MULTI_CONTROL_VOLUME_REC + SPDIF_LEFT, parentTabColumn, NULL);
	}
	
	for (i = 0; i < card->nb_ADC; i += 2) {
		parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[(i / 2) + 12]);
		create_channel_control(&mmc, ICE1712_MULTI_CONTROL_VOLUME_REC + i, parentTabColumn, NULL);
	}

	if (card->spdif_config & NO_IN_YES_OUT) {
		parentTabColumn = create_group_control(&mmc, nb_control_created, parentTab, S_null, string_list[16]);
		create_channel_control(&mmc, ICE1712_MULTI_CONTROL_VOLUME_PB + SPDIF_LEFT, parentTabColumn, NULL);
	}

	TRACE("Return %ld control(s)\n", nb_control_created);
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

	if (ICE1712_VERY_VERBOSE) {
		TRACE("flags = %#x\n", (int)data->flags);
		TRACE("request_playback_buffers = %#x\n", (int)data->request_playback_buffers);
		TRACE("request_playback_channels = %#x\n", (int)data->request_playback_channels);
		TRACE("request_playback_buffer_size = %#x\n", (int)data->request_playback_buffer_size);
		TRACE("request_record_buffers = %#x\n", (int)data->request_record_buffers);
		TRACE("request_record_channels = %#x\n", (int)data->request_record_channels);
		TRACE("request_record_buffer_size = %#x\n", (int)data->request_record_buffer_size);
	} //if (ICE1712_VERY_VERBOSE)

	// MIN_BUFFER_FRAMES <= requested value <= MAX_BUFFER_FRAMES
	card->buffer_size =	data->request_playback_buffer_size <= MAX_BUFFER_FRAMES ?	\
						data->request_playback_buffer_size >= MIN_BUFFER_FRAMES ?	\
						data->request_playback_buffer_size : MIN_BUFFER_FRAMES		\
							: MAX_BUFFER_FRAMES;

	// MIN_BUFFER_FRAMES <= requested value <= MAX_BUFFER_FRAMES
	card->buffer_size =	data->request_record_buffer_size <= MAX_BUFFER_FRAMES ?	\
						data->request_record_buffer_size >= MIN_BUFFER_FRAMES ?	\
						data->request_record_buffer_size : MIN_BUFFER_FRAMES	\
							: MAX_BUFFER_FRAMES;

	for (buff = 0; buff < SWAPPING_BUFFERS; buff++) {
		uint32 stride_o = MAX_DAC * SAMPLE_SIZE;
		uint32 stride_i = MAX_ADC * SAMPLE_SIZE;
		uint32 buf_o = stride_o * card->buffer_size;
		uint32 buf_i = stride_i * card->buffer_size;
		
		if (data->request_playback_channels == card->total_output_channels) {
			for (chan_o = 0; chan_o < card->nb_DAC; chan_o++) {
			//Analog STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb 
					+ buf_o * buff + SAMPLE_SIZE * chan_o;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				if (ICE1712_VERY_VERBOSE)
					TRACE("pb_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_o, (int)data->playback_buffers[buff][chan_o].base);
			}

			if (card->spdif_config & NO_IN_YES_OUT) {
			//SPDIF STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb
					+ buf_o * buff + SAMPLE_SIZE * SPDIF_LEFT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				if (ICE1712_VERY_VERBOSE)
					TRACE("pb_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_o, (int)data->playback_buffers[buff][chan_o].base);

				chan_o++;
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb
					+ buf_o * buff + SAMPLE_SIZE * SPDIF_RIGHT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				if (ICE1712_VERY_VERBOSE)
					TRACE("pb_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_o, (int)data->playback_buffers[buff][chan_o].base);
				chan_o++;
			}
		}
		
		if (data->request_record_channels == card->total_input_channels) {
			for (chan_i = 0; chan_i < card->nb_ADC; chan_i++) {
			//Analog STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * chan_i;
				data->record_buffers[buff][chan_i].stride = stride_i;
				if (ICE1712_VERY_VERBOSE)
					TRACE("rec_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_i, (int)data->record_buffers[buff][chan_i].base);
			}

			if (card->spdif_config & YES_IN_NO_OUT) {
			//SPDIF STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * SPDIF_LEFT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				if (ICE1712_VERY_VERBOSE)
					TRACE("rec_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_i, (int)data->record_buffers[buff][chan_i].base);

				chan_i++;
				data->record_buffers[buff][chan_i].base = card->log_addr_rec
					+ buf_i * buff + SAMPLE_SIZE * SPDIF_RIGHT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				if (ICE1712_VERY_VERBOSE)
					TRACE("rec_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_i, (int)data->record_buffers[buff][chan_i].base);
				chan_i++;
			}
			
			//The digital mixer output
			data->record_buffers[buff][chan_i].base = card->log_addr_rec
				+ buf_i * buff + SAMPLE_SIZE * MIXER_OUT_LEFT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			if (ICE1712_VERY_VERBOSE)
				TRACE("rec_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_i, (int)data->record_buffers[buff][chan_i].base);
			
			chan_i++;
			data->record_buffers[buff][chan_i].base = card->log_addr_rec
				+ buf_i * buff + SAMPLE_SIZE * MIXER_OUT_RIGHT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			if (ICE1712_VERY_VERBOSE)
				TRACE("rec_buffer[%d][%d] = %#x\n", (int)buff, (int)chan_i, (int)data->record_buffers[buff][chan_i].base);
			chan_i++;
		}
	}
	
	data->return_playback_buffers = SWAPPING_BUFFERS;
	data->return_playback_channels = card->total_output_channels;
	data->return_playback_buffer_size = card->buffer_size;
	
	TRACE("return_playback_buffers = %#x\n", (int)data->return_playback_buffers);
	TRACE("return_playback_channels = %#x\n", (int)data->return_playback_channels);
	TRACE("return_playback_buffer_size = %#x\n", (int)data->return_playback_buffer_size);

	data->return_record_buffers = SWAPPING_BUFFERS;
	data->return_record_channels = card->total_input_channels;
	data->return_record_channels = chan_i;
	data->return_record_buffer_size = card->buffer_size;
	
	TRACE("return_record_buffers = %#x\n",(int)data->return_record_buffers);
	TRACE("return_record_channels = %#x\n", (int)data->return_record_channels);
	TRACE("return_record_buffer_size = %#x\n", (int)data->return_record_buffer_size);
	
	start_DMA(card);

	return B_OK;
}


status_t 
ice1712_buffer_exchange(ice1712 *card, multi_buffer_info *data)
{
	int cpu_status = 0, status;
/*	unsigned char peak[MAX_ADC];
	int i;
*/
//	TRACE("Avant Exchange p : %d, r : %d\n", data->playback_buffer_cycle, data->record_buffer_cycle);

	status = acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000);
	switch (status) {
		case B_NO_ERROR:
//			TRACE("B_NO_ERROR\n");
			cpu_status  = disable_interrupts();

			// Playback buffers info
			data->played_real_time		= card->played_time;
			data->played_frames_count	= card->frames_count;
			data->playback_buffer_cycle	= (card->buffer - 1) % SWAPPING_BUFFERS; //Buffer played

			// Record buffers info
			data->recorded_real_time	= card->played_time;
			data->recorded_frames_count	= card->frames_count;
			data->record_buffer_cycle	= (card->buffer - 1) % SWAPPING_BUFFERS; //Buffer filled

			data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

			restore_interrupts(cpu_status);
			break;

		case B_BAD_SEM_ID:
			TRACE("B_BAD_SEM_ID\n");
			break;

		case B_INTERRUPTED:
			TRACE("B_INTERRUPTED\n");
			break;

		case B_BAD_VALUE:
			TRACE("B_BAD_VALUE\n");
			break;

		case B_WOULD_BLOCK:
			TRACE("B_WOULD_BLOCK\n");
			break;

		case B_TIMED_OUT:
			TRACE("B_TIMED_OUT\n");
			start_DMA(card);

			cpu_status = lock();

			data->played_real_time = card->played_time;
			data->playback_buffer_cycle	= 0;
			data->played_frames_count += card->buffer_size;

			data->recorded_real_time = card->played_time;
			data->record_buffer_cycle = 0;
			data->recorded_frames_count	+= card->buffer_size;
			data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

			unlock(cpu_status);
			break;

		default:
			TRACE("Default\n");
			break;
	}

	if ((card->buffer % 1500) == 0) {
		uint8 reg8, reg8_dir;
		reg8 = read_gpio(card);
		reg8_dir = read_cci_uint8(card, CCI_GPIO_DIRECTION_CONTROL);
		TRACE("DEBUG === GPIO = %d (dir %d)\n", reg8, reg8_dir);
		
		reg8 = spdif_read(card, CS84xx_VERSION_AND_CHIP_ID);
		TRACE("DEBUG === S/PDif Version : 0x%x\n", reg8);
	}

	return B_OK;
}


status_t ice1712_buffer_force_stop(ice1712 *card)
{
	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);
	return B_OK;
}
