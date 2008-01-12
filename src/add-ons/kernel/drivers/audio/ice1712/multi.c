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

#define AUTORIZED_RATE (B_SR_SAME_AS_INPUT | B_SR_IS_GLOBAL | B_SR_96000 | B_SR_88200 | B_SR_48000 | B_SR_44100)
#define AUTORIZED_SAMPLE_SIZE (B_FMT_32BIT)

static void 
start_DMA(ice1712 *card)
{
	uint16 size = card->output_buffer_size * MAX_DAC;

	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);

	write_mt_uint32(card, MT_PROF_PB_DMA_BASE_ADDRESS,		(uint32)card->phys_addr_pb);
	write_mt_uint16(card, MT_PROF_PB_DMA_COUNT_ADDRESS,		(size * SWAPPING_BUFFERS) - 1);
	//We want interrupt only from playback
	write_mt_uint16(card, MT_PROF_PB_DMA_TERM_COUNT,		size - 1);
	TRACE_ICE(("SIZE DMA PLAYBACK %#x\n", size));

	size = card->input_buffer_size * MAX_ADC;

	write_mt_uint32(card, MT_PROF_REC_DMA_BASE_ADDRESS,		(uint32)card->phys_addr_rec);
	write_mt_uint16(card, MT_PROF_REC_DMA_COUNT_ADDRESS,	(size * SWAPPING_BUFFERS) - 1);
	//We do not want any interrupt from the record
	write_mt_uint16(card, MT_PROF_REC_DMA_TERM_COUNT,		0);
	TRACE_ICE(("SIZE DMA RECORD %#x\n", size));

	//Enable output AND Input from Analog CODEC
	switch (card->product)
	{//TODO: find correct value for all card
		case ICE1712_SUBDEVICE_DELTA66 :
		case ICE1712_SUBDEVICE_DELTA44 :
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496 :
		case ICE1712_SUBDEVICE_DELTADIO2496 :
		case ICE1712_SUBDEVICE_DELTA410 :
		case ICE1712_SUBDEVICE_DELTA1010LT :
		case ICE1712_SUBDEVICE_DELTA1010 :
			codec_write(card, AK45xx_CLOCK_FORMAT_REGISTER, 0x69);
			codec_write(card, AK45xx_RESET_REGISTER, 0x03);
			break;
		case ICE1712_SUBDEVICE_VX442 :
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_1);
			break;
	}

	//Set Data Format for SPDif codec
	switch (card->product)
	{//TODO: find correct value for all card
		case ICE1712_SUBDEVICE_DELTA1010 :
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496 :
			break;
		case ICE1712_SUBDEVICE_DELTA66 :
		case ICE1712_SUBDEVICE_DELTA44 :
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_1);
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496 :
			spdif_write(card, CS84xx_SERIAL_INPUT_FORMAT_REG, 0x85);
			spdif_write(card, CS84xx_SERIAL_OUTPUT_FORMAT_REG, 0x85);
//			spdif_write(card, CS84xx_SERIAL_OUTPUT_FORMAT_REG, 0x41);
			break;
		case ICE1712_SUBDEVICE_DELTA410 :
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT :
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_0);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_1);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_2);
//			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_3);
			break;
		case ICE1712_SUBDEVICE_VX442 :
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

	switch (card->product)
	{
		case ICE1712_SUBDEVICE_DELTA1010 :
			strncpy(data->friendly_name, "Delta 1010", 32);
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496 :
			strncpy(data->friendly_name, "Delta DIO 2496", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA66 :
			strncpy(data->friendly_name, "Delta 66", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA44 :
			strncpy(data->friendly_name, "Delta 44", 32);
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496 :
			strncpy(data->friendly_name, "Audiophile 2496", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA410 :
			strncpy(data->friendly_name, "Delta 410", 32);
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT :
			strncpy(data->friendly_name, "Delta 1010 LT", 32);
			break;
		case ICE1712_SUBDEVICE_VX442 :
			strncpy(data->friendly_name, "VX 442", 32);
			break;

		default :
			strncpy(data->friendly_name, "Unknow Device", 32);
			break;
	}
			
	strncpy(data->vendor_info, "J. H Leveque", 32);
	
	data->output_channel_count		= card->total_output_channels;
	data->input_channel_count		= card->total_input_channels;
	data->output_bus_channel_count	= 0;
	data->input_bus_channel_count	= 0;
	data->aux_bus_channel_count		= 0;


	TRACE_ICE(("request_channel_count = %d\n", data->request_channel_count));
	if (data->request_channel_count >= (card->total_output_channels + card->total_input_channels)) {
		for (i = 0; i < card->nb_DAC; i++) {
		//Analog STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | \
									((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		if (card->spdif_config & NO_IN_YES_OUT) {
		//SPDIF STEREO output
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_LEFT;
			data->channels[chan].connectors = 0;
			chan++;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_OUTPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		for (i = 0; i < card->nb_ADC; i++) {
		//Analog STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | \
									((i & 1) == 0) ? B_CHANNEL_LEFT : B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		if (card->spdif_config & YES_IN_NO_OUT) {
		//SPDIF STEREO input
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_LEFT;
			data->channels[chan].connectors = 0;
			chan++;
			data->channels[chan].channel_id = chan;
			data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
			data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_RIGHT;
			data->channels[chan].connectors = 0;
			chan++;
		}

		//The digital mixer output
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_LEFT;
		data->channels[chan].connectors = 0;
		chan++;
		data->channels[chan].channel_id = chan;
		data->channels[chan].kind = B_MULTI_INPUT_CHANNEL;
		data->channels[chan].designations = B_CHANNEL_STEREO_BUS | B_CHANNEL_RIGHT;
		data->channels[chan].connectors = 0;
		chan++;
	}

	TRACE_ICE(("output_channel_count = %d\n", data->output_channel_count));
	TRACE_ICE(("input_channel_count = %d\n", data->input_channel_count));
	TRACE_ICE(("output_bus_channel_count = %d\n", data->output_bus_channel_count));
	TRACE_ICE(("input_bus_channel_count = %d\n", data->input_bus_channel_count));

	data->output_rates = data->input_rates = AUTORIZED_RATE;
	data->min_cvsr_rate = 44100;
	data->max_cvsr_rate = 96000;

	data->output_formats = data->input_formats = AUTORIZED_SAMPLE_SIZE;
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
		TRACE_ICE(("set_enabled_channels %d : %s\n", i, B_TEST_CHANNEL(data->enable_bits, i) ? "enabled": "disabled"));

	TRACE_ICE(("lock_source %d\n", data->lock_source));
	TRACE_ICE(("lock_data %d\n", data->lock_data));
	
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
	
	switch (sr)
	{
		case 0x0 :
			data->input.rate = data->output.rate = B_SR_48000;
			data->input.cvsr = data->output.cvsr = 48000.0f;
			break;
		case 0x7 :
			data->input.rate = data->output.rate = B_SR_96000;
			data->input.cvsr = data->output.cvsr = 96000.0f;
			break;
		case 0x8 :
			data->input.rate = data->output.rate = B_SR_44100;
			data->input.cvsr = data->output.cvsr = 44100.0f;
			break;
		case 0xB :
			data->input.rate = data->output.rate = B_SR_88200;
			data->input.cvsr = data->output.cvsr = 88200.0f;
			break;
	}
	
	data->timecode_kind = 0;
	data->output_latency = data->input_latency = 0;
	data->output.format = data->input.format = AUTORIZED_SAMPLE_SIZE;

	TRACE_ICE(("Input Sampling Rate = %d\n", (int)data->input.cvsr));
	TRACE_ICE(("Output Sampling Rate = %d\n", (int)data->output.cvsr));

	return B_OK;
}


status_t 
ice1712_set_global_format(ice1712 *card, multi_format_info *data)
{
	uint8 i;

	TRACE_ICE(("Input Sampling Rate = %#x\n", data->input.rate));
	TRACE_ICE(("Output Sampling Rate = %#x\n", data->output.rate));

	if ((data->input.rate == data->output.rate) &&
		(card->lock_source == B_MULTI_LOCK_INTERNAL)) {
		switch (data->input.rate)
		{
			case B_SR_96000 :
				card->sampling_rate = 0x07;
				break;
			case B_SR_48000 :
				card->sampling_rate = 0x00;
				break;
			case B_SR_88200 :
				card->sampling_rate = 0x0B;
				break;
			case B_SR_44100 :
				card->sampling_rate = 0x08;
				break;
		}
		write_mt_uint8(card, MT_SAMPLING_RATE_SELECT, card->sampling_rate);
	}
	i = read_mt_uint8(card, MT_SAMPLING_RATE_SELECT);
	TRACE_ICE(("RATE = %#x\n", i));
	return B_OK;
}


status_t 
ice1712_get_mix(ice1712 *card, multi_mix_value_info *data)
{//Not Implemented
	return B_ERROR;
}


status_t 
ice1712_set_mix(ice1712 *card, multi_mix_value_info *data)
{//Not Implemented
	return B_ERROR;
}


status_t 
ice1712_list_mix_channels(ice1712 *card, multi_mix_channel_info *data)
{//Not Implemented
	return B_ERROR;
}


#define MULTI_AUDIO_BASE_ID 1024
#define MULTI_AUDIO_MASTER_ID 0

#ifndef B_MULTI_MIX_GROUP
#define B_MULTI_MIX_GROUP 10
#endif

#ifndef S_null
#define S_null 10
#endif

#ifndef S_STEREO_MIX
#define S_STEREO_MIX 10
#endif

static int32
create_group_control(multi_mix_control *multi, int32 idx, int32 parent, int32 string, const char* name)
{
	multi->id = MULTI_AUDIO_BASE_ID + idx;
	multi->master = parent;
	multi->flags = B_MULTI_MIX_GROUP;
	multi->master = MULTI_AUDIO_MASTER_ID;
//	multi->string = string;

	if (name != NULL)
		strcpy(multi->name, name);
	
	return multi->id;
}

static int32
create_slider_control(multi_mix_control *multi, int32 idx, int32 parent, int32 string, const char* name)
{
	multi->id = MULTI_AUDIO_BASE_ID + idx;
	multi->master = parent;
	multi->flags = B_MULTI_MIX_GAIN;
	multi->master = MULTI_AUDIO_MASTER_ID + 1;
//	multi->string = string;

	multi->u.gain.min_gain		= -144.0;
	multi->u.gain.max_gain		= 0.0;
	multi->u.gain.granularity	= 1.5;

	if (name != NULL)
		strcpy(multi->name, name);
	
	return multi->id;
}


status_t
ice1712_list_mix_controls(ice1712 *card, multi_mix_control_info *data)
{
	int32 parent;

	TRACE_ICE(("count = %d\n", data->control_count));
	parent = create_group_control	(data->controls + 0, 0, 0, S_null, "Playback");
	create_slider_control			(data->controls + 1, 1, parent, S_STEREO_MIX, "Master Output");

	parent = create_group_control	(data->controls + 2, 2, 0, S_null, "Record");
	create_slider_control			(data->controls + 3, 3, parent, S_STEREO_MIX, "Master Input");
	data->control_count = 4;

	return B_OK;
}


status_t 
ice1712_list_mix_connections(ice1712 *card, multi_mix_connection_info *data)
{//Not Implemented
	return B_ERROR;
}


status_t 
ice1712_get_buffers(ice1712 *card, multi_buffer_list *data)
{
	int buff, chan_i = 0, chan_o = 0;

	TRACE_ICE(("flags = %#x\n",data->flags));
	TRACE_ICE(("request_playback_buffers = %#x\n",		data->request_playback_buffers));
	TRACE_ICE(("request_playback_channels = %#x\n",		data->request_playback_channels));
	TRACE_ICE(("request_playback_buffer_size = %#x\n",	data->request_playback_buffer_size));
	TRACE_ICE(("request_record_buffers = %#x\n",		data->request_record_buffers));
	TRACE_ICE(("request_record_channels = %#x\n",		data->request_record_channels));
	TRACE_ICE(("request_record_buffer_size = %#x\n",	data->request_record_buffer_size));

	// MIN_BUFFER_FRAMES <= requested value <= MAX_BUFFER_FRAMES
	card->output_buffer_size =	data->request_playback_buffer_size <= MAX_BUFFER_FRAMES ?	\
								data->request_playback_buffer_size >= MIN_BUFFER_FRAMES ?	\
								data->request_playback_buffer_size : MIN_BUFFER_FRAMES		\
								: MAX_BUFFER_FRAMES;

	// MIN_BUFFER_FRAMES <= requested value <= MAX_BUFFER_FRAMES
	card->input_buffer_size =	data->request_record_buffer_size <= MAX_BUFFER_FRAMES ?	\
								data->request_record_buffer_size >= MIN_BUFFER_FRAMES ?	\
								data->request_record_buffer_size : MIN_BUFFER_FRAMES	\
								: MAX_BUFFER_FRAMES;

	for (buff = 0; buff < SWAPPING_BUFFERS; buff++) {
		uint32 stride_o = MAX_DAC * SAMPLE_SIZE;
		uint32 stride_i = MAX_ADC * SAMPLE_SIZE;
		uint32 buf_o = stride_o * card->output_buffer_size;
		uint32 buf_i = stride_i * card->input_buffer_size;
		
		if (data->request_playback_channels == card->total_output_channels) {
			for (chan_o = 0; chan_o < card->nb_DAC; chan_o++) {
			//Analog STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb + \
															buf_o * buff + \
															SAMPLE_SIZE * chan_o;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_ICE(("pb_buffer[%d][%d] = %#x\n", \
									buff, chan_o, data->playback_buffers[buff][chan_o].base));
			}

			if (card->spdif_config & NO_IN_YES_OUT) {
			//SPDIF STEREO output
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb + \
															buf_o * buff + \
															SAMPLE_SIZE * SPDIF_LEFT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_ICE(("pb_buffer[%d][%d] = %#x\n", \
									buff, chan_o, data->playback_buffers[buff][chan_o].base));

				chan_o++;
				data->playback_buffers[buff][chan_o].base = card->log_addr_pb + \
															buf_o * buff + \
															SAMPLE_SIZE * SPDIF_RIGHT;
				data->playback_buffers[buff][chan_o].stride = stride_o;
				TRACE_ICE(("pb_buffer[%d][%d] = %#x\n", \
									buff, chan_o, data->playback_buffers[buff][chan_o].base));
				chan_o++;
			}
		}
		
		if (data->request_record_channels == card->total_input_channels) {
			for (chan_i = 0; chan_i < card->nb_ADC; chan_i++) {
			//Analog STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec + \
															buf_i * buff + \
															SAMPLE_SIZE * chan_i;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_ICE(("rec_buffer[%d][%d] = %#x\n", \
									buff, chan_i, data->record_buffers[buff][chan_i].base));
			}

			if (card->spdif_config & YES_IN_NO_OUT) {
			//SPDIF STEREO input
				data->record_buffers[buff][chan_i].base = card->log_addr_rec + \
														buf_i * buff + \
														SAMPLE_SIZE * SPDIF_LEFT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_ICE(("rec_buffer[%d][%d] = %#x\n", \
									buff, chan_i, data->record_buffers[buff][chan_i].base));

				chan_i++;
				data->record_buffers[buff][chan_i].base = card->log_addr_rec + \
														buf_i * buff + \
														SAMPLE_SIZE * SPDIF_RIGHT;
				data->record_buffers[buff][chan_i].stride = stride_i;
				TRACE_ICE(("rec_buffer[%d][%d] = %#x\n", \
									buff, chan_i, data->record_buffers[buff][chan_i].base));
				chan_i++;
			}
			
			//The digital mixer output
			data->record_buffers[buff][chan_i].base = card->log_addr_rec + \
													buf_i * buff + \
													SAMPLE_SIZE * MIXER_OUT_LEFT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			TRACE_ICE(("rec_buffer[%d][%d] = %#x\n", \
									buff, chan_i, data->record_buffers[buff][chan_i].base));
			
			chan_i++;
			data->record_buffers[buff][chan_i].base = card->log_addr_rec + \
													buf_i * buff + \
													SAMPLE_SIZE * MIXER_OUT_RIGHT;
			data->record_buffers[buff][chan_i].stride = stride_i;
			TRACE_ICE(("rec_buffer[%d][%d] = %#x\n", \
									buff, chan_i, data->record_buffers[buff][chan_i].base));
			chan_i++;
		}
	}
	
	data->return_playback_buffers = SWAPPING_BUFFERS;
	data->return_playback_channels = card->total_output_channels;
	data->return_playback_buffer_size = card->output_buffer_size;
	
	TRACE_ICE(("return_playback_buffers = %#x\n",		data->return_playback_buffers));
	TRACE_ICE(("return_playback_channels = %#x\n",		data->return_playback_channels));
	TRACE_ICE(("return_playback_buffer_size = %#x\n",	data->return_playback_buffer_size));

	data->return_record_buffers = SWAPPING_BUFFERS;
	data->return_record_channels = card->total_input_channels;
	data->return_record_channels = chan_i;
	data->return_record_buffer_size = card->input_buffer_size;
	
	TRACE_ICE(("return_record_buffers = %#x\n",			data->return_record_buffers));
	TRACE_ICE(("return_record_channels = %#x\n",		data->return_record_channels));
	TRACE_ICE(("return_record_buffer_size = %#x\n",		data->return_record_buffer_size));
	
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
//	TRACE_ICE(("Avant Exchange p : %d, r : %d\n", data->playback_buffer_cycle, data->record_buffer_cycle));

	status = acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000);
	switch (status)
	{
		case B_NO_ERROR :
//			TRACE_ICE(("B_NO_ERROR\n"));
			cpu_status  = lock();

			// do playback
			data->played_real_time		= card->played_time;
			data->played_frames_count	+= card->output_buffer_size;
			data->playback_buffer_cycle	= (card->buffer/* - 1*/) & 0x1;	//Buffer TO fill

			// do record
			data->recorded_real_time	= card->played_time;
			data->recorded_frames_count	+= card->input_buffer_size;
			data->record_buffer_cycle	= (card->buffer/* - 1*/) & 0x1;	//Buffer filled

			data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

			unlock(cpu_status);
			break;

		case B_BAD_SEM_ID :
			TRACE_ICE(("B_BAD_SEM_ID\n"));
			break;

		case B_INTERRUPTED :
			TRACE_ICE(("B_INTERRUPTED\n"));
			break;

		case B_BAD_VALUE :
			TRACE_ICE(("B_BAD_VALUE\n"));
			break;

		case B_WOULD_BLOCK :
			TRACE_ICE(("B_WOULD_BLOCK\n"));
			break;

		case B_TIMED_OUT :
			TRACE_ICE(("B_TIMED_OUT\n"));
			start_DMA(card);

			cpu_status  = lock();

			data->played_real_time		= card->played_time;
			data->playback_buffer_cycle	= 0;
			data->played_frames_count	+= card->output_buffer_size;

			data->recorded_real_time	= card->played_time;
			data->record_buffer_cycle	= 0;
			data->recorded_frames_count	+= card->input_buffer_size;
			data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

			unlock(cpu_status);
			break;

		default :
			TRACE_ICE(("Default\n"));
			break;
	}

/*	if ((card->buffer % 500) == 0)
	{
		uint8 reg8, reg8_dir;
		reg8 = read_gpio(card);
		reg8_dir = read_cci_uint8(card, CCI_GPIO_DIRECTION_CONTROL);
		TRACE_ICE(("B_NO_ERROR buffer = %d : GPIO = %d (%d)\n", card->buffer, reg8, reg8_dir));
	}
*/
	return B_OK;
}

status_t ice1712_buffer_force_stop(ice1712 *card)
{
	write_mt_uint8(card, MT_PROF_PB_CONTROL, 0);
	return B_OK;
}

