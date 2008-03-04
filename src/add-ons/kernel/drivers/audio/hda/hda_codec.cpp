/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "driver.h"
#include "hda_codec_defs.h"


static const char* kPortConnector[] = {
	"Jack", "None", "Fixed", "Dual"
};

static const char* kDefaultDevice[] = {
	"Line Out", "Speaker", "HP Out", "CD", "SPDIF out", "Digital Other Out",
	"Modem Line Side", "Modem Hand Side", "Line In", "AUX", "Mic In",
	"Telephony", "SPDIF In", "Digital Other In", "Reserved", "Other"
};

static const char* kConnectionType[] = {
	"N/A", "1/8\"", "1/4\"", "ATAPI internal", "RCA", "Optical",
	"Other Digital", "Other Analog", "Multichannel Analog (DIN)",
	"XLR/Professional", "RJ-11 (Modem)", "Combination", "-", "-", "-", "Other"
};

static const char* kJackColor[] = {
	"N/A", "Black", "Grey", "Blue", "Green", "Red", "Orange", "Yellow",
	"Purple", "Pink", "-", "-", "-", "-", "White", "Other"
};


static status_t
hda_widget_get_pm_support(hda_codec* codec, uint32 nodeID, uint32* pm)
{
	corb_t verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_POWERSTATE_SUPPORT);
	status_t rc;
	uint32 resp;

	if ((rc = hda_send_verbs(codec, &verb, &resp, 1)) == B_OK) {
		*pm = 0;

		/* FIXME: Define constants for powermanagement modes */
		if (resp & (1 << 0))	;
		if (resp & (1 << 1))	;
		if (resp & (1 << 2))	;
		if (resp & (1 << 3))	;
	}

	return rc;
}


static status_t
hda_widget_get_stream_support(hda_codec* codec, uint32 nodeID, uint32* formats,
	uint32* rates)
{
	corb_t verbs[2];
	uint32 resp[2];
	status_t status;

	verbs[0] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_STREAM_SUPPORT);
	verbs[1] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_PCM_SUPPORT);

	status = hda_send_verbs(codec, verbs, resp, 2);
	if (status != B_OK)
		return status;

	*formats = 0;
	*rates = 0;

	if ((resp[0] & (STREAM_FLOAT | STREAM_PCM)) != 0) {
		if (resp[1] & (1 << 0))
			*rates |= B_SR_8000;
		if (resp[1] & (1 << 1))
			*rates |= B_SR_11025;
		if (resp[1] & (1 << 2))
			*rates |= B_SR_16000;
		if (resp[1] & (1 << 3))
			*rates |= B_SR_22050;
		if (resp[1] & (1 << 4))
			*rates |= B_SR_32000;
		if (resp[1] & (1 << 5))
			*rates |= B_SR_44100;
		if (resp[1] & (1 << 6))
			*rates |= B_SR_48000;
		if (resp[1] & (1 << 7))
			*rates |= B_SR_88200;
		if (resp[1] & (1 << 8))
			*rates |= B_SR_96000;
		if (resp[1] & (1 << 9))
			*rates |= B_SR_176400;
		if (resp[1] & (1 << 10))
			*rates |= B_SR_192000;
		if (resp[1] & (1 << 11))
			*rates |= B_SR_384000;

		if (resp[1] & PCM_8_BIT)
			*formats |= B_FMT_8BIT_S;
		if (resp[1] & PCM_16_BIT)
			*formats |= B_FMT_16BIT;
		if (resp[1] & PCM_20_BIT)
			*formats |= B_FMT_20BIT;
		if (resp[1] & PCM_24_BIT)
			*formats |= B_FMT_24BIT;
		if (resp[1] & PCM_32_BIT)
			*formats |= B_FMT_32BIT;
	}
	if ((resp[0] & STREAM_FLOAT) != 0)
		*formats |= B_FMT_FLOAT;

//FIXME:	if (resp[0] & (1 << 2))	/* Sort out how to handle AC3 */;

	return B_OK;
}


static status_t
hda_widget_get_amplifier_capabilities(hda_codec* codec, uint32 nodeID)
{
	status_t rc;
	corb_t verb;
	uint32 resp;

	verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_OUTPUT_AMPLIFIER_CAP);
	rc = hda_send_verbs(codec, &verb, &resp, 1);
	if (rc == B_OK && resp != 0) {
		dprintf("\tAMP: Mute: %s, step size: %ld, # steps: %ld, offset: %ld\n",
			(resp & (1 << 31)) ? "supported" : "N/A",
			(resp >> 16) & 0x7F,
			(resp >> 8) & 0x7F,
			resp & 0x7F);
	}

	return rc;
}


static const char*
get_widget_type_name(hda_widget_type type)
{
	switch (type) {
		case WT_AUDIO_OUTPUT:
			return "Audio Output";
		case WT_AUDIO_INPUT:
			return "Audio Input";
		case WT_AUDIO_MIXER:
			return "Audio Mixer";
		case WT_AUDIO_SELECTOR:
			return "Audio Selector";
		case WT_PIN_COMPLEX:
			return "Pin Complex";
		case WT_POWER:
			return "Power";
		case WT_VOLUME_KNOB:
			return "Volume Knob";
		case WT_BEEP_GENERATOR:
			return "Beep Generator";
		case WT_VENDOR_DEFINED:
			return "Vendor Defined";
		default:
			return "Unknown";
	}
}


static status_t
hda_codec_parse_audio_group(hda_audio_group* audioGroup)
{
	corb_t verbs[3];
	uint32 resp[3];

	hda_widget_get_stream_support(audioGroup->codec, audioGroup->root_node_id,
		&audioGroup->supported_formats, &audioGroup->supported_rates);
	hda_widget_get_pm_support(audioGroup->codec, audioGroup->root_node_id,
		&audioGroup->supported_pm);

	verbs[0] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAMETER, PID_AUDIO_GROUP_CAP);
	verbs[1] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAMETER, PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAMETER, PID_SUBORDINATE_NODE_COUNT);

	if (hda_send_verbs(audioGroup->codec, verbs, resp, 3) != B_OK)
		return B_ERROR;

	dprintf("hda: Audio Group: Output delay: %ld samples, Input delay: %ld "
		"samples, Beep Generator: %s\n", resp[0] & 0xf,
		(resp[0] >> 8) & 0xf, (resp[0] & (1 << 16)) ? "yes" : "no");

	dprintf("hda:   #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n",
		resp[4] & 0xff, (resp[1] >> 8) & 0xff,
		(resp[1] >> 16) & 0xff, (resp[1] & (1 << 30)) ? "yes" : "no",
		(resp[1] & (1 << 31)) ? "yes" : "no");

	audioGroup->widget_start = resp[2] >> 16;
	audioGroup->widget_count = resp[2] & 0xff;

	dprintf("hda:   widget start %lu, count %lu\n", audioGroup->widget_start,
		audioGroup->widget_count);

	audioGroup->widgets = (hda_widget*)calloc(audioGroup->widget_count,
		sizeof(*audioGroup->widgets));
	if (audioGroup->widgets == NULL) {
		dprintf("ERROR: Not enough memory!\n");
		return B_NO_MEMORY;
	}

	/* Iterate over all Widgets and collect info */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		uint32 widget = audioGroup->widget_start + i;

		verbs[0] = MAKE_VERB(audioGroup->codec->addr, widget, VID_GET_PARAMETER,
			PID_AUDIO_WIDGET_CAP);
		verbs[1] = MAKE_VERB(audioGroup->codec->addr, widget, VID_GET_PARAMETER,
			PID_CONNECTION_LIST_LENGTH);
		if (hda_send_verbs(audioGroup->codec, verbs, resp, 2) != B_OK)
			return B_ERROR;

		audioGroup->widgets[i].type = (hda_widget_type)(resp[0] >> 20);
		audioGroup->widgets[i].num_inputs = resp[1] & 0x7f;

		dprintf("%ld: %s\n", widget,
			get_widget_type_name(audioGroup->widgets[i].type));

		char buf[256];
		int off = 0;
		buf[0] = '\0';

		if (resp[0] & (1 << 11))
			off += sprintf(buf + off, "[L-R Swap] ");

		if (resp[0] & (1 << 10)) {
			corb_t verb;
			uint32 resp;

			off += sprintf(buf + off, "[Power] ");

			/* We support power; switch us on! */
			verb = MAKE_VERB(audioGroup->codec->addr, widget,
				VID_SET_POWER_STATE, 0);
			hda_send_verbs(audioGroup->codec, &verb, &resp, 1);
		}

		if (resp[0] & (1 << 9))
			off += sprintf(buf + off, "[Digital] ");
		if (resp[0] & (1 << 7))
			off += sprintf(buf + off, "[Unsol Capable] ");
		if (resp[0] & (1 << 6))
			off += sprintf(buf + off, "[Proc Widget] ");
		if (resp[0] & (1 << 5))
			off += sprintf(buf + off, "[Stripe] ");
		if (resp[0] & (1 << 4))
			off += sprintf(buf + off, "[Format Override] ");
		if (resp[0] & (1 << 3))
			off += sprintf(buf + off, "[Amp Param Override] ");
		if (resp[0] & (1 << 2))
			off += sprintf(buf + off, "[Out Amp] ");
		if (resp[0] & (1 << 1))
			off += sprintf(buf + off, "[In Amp] ");
		if (resp[0] & (1 << 0))
			off += sprintf(buf + off, "[Stereo] ");

		switch (audioGroup->widgets[i].type) {
			case WT_AUDIO_OUTPUT:
				hda_widget_get_stream_support(audioGroup->codec, widget,
					&audioGroup->widgets[i].d.input.formats,
					&audioGroup->widgets[i].d.input.rates);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, widget);
				break;
			case WT_AUDIO_INPUT:
				hda_widget_get_stream_support(audioGroup->codec, widget,
					&audioGroup->widgets[i].d.input.formats,
					&audioGroup->widgets[i].d.input.rates);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, widget);
				break;
			case WT_AUDIO_MIXER:
				hda_widget_get_amplifier_capabilities(audioGroup->codec, widget);
				break;
			case WT_AUDIO_SELECTOR:
				hda_widget_get_amplifier_capabilities(audioGroup->codec, widget);
				break;
			case WT_PIN_COMPLEX:
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, widget,
					VID_GET_PARAMETER, PID_PIN_CAP);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					audioGroup->widgets[i].d.pin.input = resp[0] & (1 << 5);
					audioGroup->widgets[i].d.pin.output = resp[0] & (1 << 4);

					dprintf("\t%s%s\n", 
						audioGroup->widgets[i].d.pin.input ? "[Input] " : "",
						audioGroup->widgets[i].d.pin.output ? "[Output]" : "");
				} else {
					dprintf("%s: Error getting Pin Complex IO\n", __func__);
				}

				verbs[0] = MAKE_VERB(audioGroup->codec->addr, widget,
					VID_GET_CONFIGURATION_DEFAULT, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					audioGroup->widgets[i].d.pin.device = (pin_dev_type)
						((resp[0] >> 20) & 0xf);
					dprintf("\t%s, %s, %s, %s\n",
						kPortConnector[resp[0] >> 30],	
						kDefaultDevice[audioGroup->widgets[i].d.pin.device],
						kConnectionType[(resp[0] >> 16) & 0xF],
						kJackColor[(resp[0] >> 12) & 0xF]);
				}

				hda_widget_get_amplifier_capabilities(audioGroup->codec, widget);
				break;

			default:
				break;
		}

		if (off)
			dprintf("\t%s\n", buf);

		hda_widget_get_pm_support(audioGroup->codec, widget,
			&audioGroup->widgets[i].pm);

		if (audioGroup->widgets[i].num_inputs) {
			if (audioGroup->widgets[i].num_inputs > 1) {
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, widget,
					VID_GET_CONNECTION_SELECT, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK)
					audioGroup->widgets[i].active_input = resp[0] & 0xff;
				else
					audioGroup->widgets[i].active_input = -1;
			} else 
				audioGroup->widgets[i].active_input = -1;

			buf[0] = '\0';
			off = 0;

			for (uint32 inputIndex = 0; inputIndex
					< audioGroup->widgets[i].num_inputs; inputIndex++) {
				if ((inputIndex % 4) == 0) {
					// we get 4 answers per call in short list mode
					verbs[0] = MAKE_VERB(audioGroup->codec->addr, widget,
						VID_GET_CONNECTION_LIST_ENTRY, inputIndex);
					if (hda_send_verbs(audioGroup->codec, verbs, resp, 1)
							!= B_OK) {
						dprintf("hda: Error parsing inputs for widget %ld!\n",
							widget);
						break;
					}
				}

				uint32 input = (resp[0] >> (8 * (inputIndex % 4))) & 0xff;

				if ((int32)inputIndex != audioGroup->widgets[i].active_input)
					off += sprintf(buf + off, "%ld ", input);
				else
					off += sprintf(buf + off, "(%ld) ", input);

				audioGroup->widgets[i].inputs[inputIndex] = input;
			}

			if (off != 0)
				dprintf("\tConnections: %s\n", buf);
		}
	}

	return B_OK;
}


/*! Find path from 'widget' to a widget of type \a widgetType, returning its
	widget id.
	Returns 0 if not found.
*/
static uint32
hda_codec_audio_group_find_path(hda_audio_group* audioGroup, uint32 widget,
	hda_widget_type widgetType, uint32 depth)
{
	int groupIndex = widget - audioGroup->widget_start;

	switch (audioGroup->widgets[groupIndex].type) {
		case WT_AUDIO_MIXER:
			for (uint32 i = 0; i < audioGroup->widgets[groupIndex].num_inputs; i++) {
				if (hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[groupIndex].inputs[i], widgetType,
						depth + 1)) {
					if (audioGroup->widgets[groupIndex].active_input == -1)
						audioGroup->widgets[groupIndex].active_input = i;

					return audioGroup->widgets[groupIndex].inputs[i];
				}
			}
			break;

		case WT_AUDIO_SELECTOR:
		{
			int32 i = audioGroup->widgets[groupIndex].active_input;
			if (i != -1) {
				widget = audioGroup->widgets[groupIndex].inputs[i];
				if (hda_codec_audio_group_find_path(audioGroup, widget,
						widgetType, depth + 1)) {
					return widget;
				}
			}
			break;
		}

		default:
			if (audioGroup->widgets[groupIndex].type == widgetType)
				return widget;

			break;
	}

	return 0;
}


static void
hda_codec_delete_audio_group(hda_audio_group* audioGroup)
{
	if (audioGroup == NULL)
		return;

	if (audioGroup->playback_stream != NULL)
		hda_stream_delete(audioGroup->playback_stream);

	if (audioGroup->record_stream != NULL)
		hda_stream_delete(audioGroup->record_stream);

	free(audioGroup->widgets);
	free(audioGroup);
}


static status_t
hda_codec_new_audio_group(hda_codec* codec, uint32 audioGroupNodeID)
{
	hda_audio_group* audioGroup = (hda_audio_group*)calloc(1,
		sizeof(hda_audio_group));
	if (audioGroup == NULL)
		return B_NO_MEMORY;

	/* Setup minimal info needed by hda_codec_parse_afg */
	audioGroup->root_node_id = audioGroupNodeID;
	audioGroup->codec = codec;

	/* Parse all widgets in Audio Function Group */
	status_t status = hda_codec_parse_audio_group(audioGroup);
	if (status != B_OK)
		goto err;

	/* Setup for worst-case scenario; we cannot find any output Pin Widgets */
	status = ENODEV;

	/* Try to locate all input/output channels */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		uint32 outputWidget = 0, inputWidget = 0;

		if (audioGroup->playback_stream == NULL
			&& audioGroup->widgets[i].type == WT_PIN_COMPLEX
			&& audioGroup->widgets[i].d.pin.output) {
			if (audioGroup->widgets[i].d.pin.device == PIN_DEV_HEAD_PHONE_OUT
				|| audioGroup->widgets[i].d.pin.device == PIN_DEV_SPEAKER
				|| audioGroup->widgets[i].d.pin.device == PIN_DEV_LINE_OUT) {
				int32 inputIndex = audioGroup->widgets[i].active_input;
				if (inputIndex != -1) {
					outputWidget = hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[i].inputs[inputIndex],
						WT_AUDIO_OUTPUT, 0);
				} else {
					// find and select output widget
					for (inputIndex = 0; (uint32)inputIndex
							< audioGroup->widgets[i].num_inputs; inputIndex++) {
						outputWidget = hda_codec_audio_group_find_path(audioGroup,
							audioGroup->widgets[i].inputs[inputIndex],
							WT_AUDIO_OUTPUT, 0);
						if (outputWidget) {
							corb_t verb = MAKE_VERB(codec->addr,
								i + audioGroup->widget_start,
								VID_SET_CONNECTION_SELECT, inputIndex);
							if (hda_send_verbs(codec, &verb, NULL, 1) != B_OK)
								dprintf("%s: Setting output selector failed!\n", __func__);
							break;
						}
					}
				}

				if (outputWidget) {
					if (!audioGroup->playback_stream) {
						corb_t verb[2];

						/* Setup playback/record streams for Multi Audio API */
						audioGroup->playback_stream = hda_stream_new(
							audioGroup->codec->controller, STREAM_PLAYBACK);
						audioGroup->record_stream = hda_stream_new(
							audioGroup->codec->controller, STREAM_RECORD);

						audioGroup->playback_stream->pin_widget = i
							+ audioGroup->widget_start;
						audioGroup->playback_stream->io_widget = outputWidget;

						/* FIXME: Force Pin Widget to unmute; enable hp/output */
						verb[0] = MAKE_VERB(codec->addr,
							audioGroup->playback_stream->pin_widget,
							VID_SET_AMPLIFIER_GAIN_MUTE,
							AMP_SET_OUTPUT | AMP_SET_LEFT_CHANNEL
								| AMP_SET_RIGHT_CHANNEL);
						verb[1] = MAKE_VERB(codec->addr,
							audioGroup->playback_stream->pin_widget,
							VID_SET_PIN_WIDGET_CONTROL,
							PIN_ENABLE_HEAD_PHONE | PIN_ENABLE_OUTPUT);
						hda_send_verbs(codec, verb, NULL, 2);

						dprintf("%s: Found output PIN (%s) connected to output "
							"CONV wid:%ld\n", __func__,
							kDefaultDevice[audioGroup->widgets[i].d.pin.device], outputWidget);
					}	
				}
			}
		}

		if (audioGroup->widgets[i].type == WT_AUDIO_INPUT) {
			int32 inputIndex = audioGroup->widgets[i].active_input;
			if (inputIndex != -1) {
				inputWidget = hda_codec_audio_group_find_path(audioGroup,
					audioGroup->widgets[i].inputs[inputIndex], WT_PIN_COMPLEX,
					0);
			} else {
				// find and select input widget
				for (inputIndex = 0; (uint32)inputIndex
						< audioGroup->widgets[i].num_inputs; inputIndex++) {
					inputWidget = hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[i].inputs[inputIndex],
						WT_PIN_COMPLEX, 0);
					if (inputWidget) {
						corb_t verb = MAKE_VERB(codec->addr,
							i + audioGroup->widget_start,
							VID_SET_CONNECTION_SELECT, inputIndex);
						if (hda_send_verbs(codec, &verb, NULL, 1) != B_OK) {
							dprintf("%s: Setting input selector failed!\n",
								__func__);
						}
						break;
					}
				}
			}

			if (inputWidget) {
				if (!audioGroup->record_stream) {
					corb_t verb;

					/* Setup playback/record streams for Multi Audio API */
					audioGroup->record_stream = hda_stream_new(
						audioGroup->codec->controller, STREAM_RECORD);

					audioGroup->record_stream->pin_widget = inputWidget;
					audioGroup->record_stream->io_widget = i
						+ audioGroup->widget_start;

					/* FIXME: Force Pin Widget to unmute */
					verb = MAKE_VERB(codec->addr,
						audioGroup->record_stream->pin_widget,
						VID_SET_AMPLIFIER_GAIN_MUTE,
						AMP_SET_INPUT | AMP_SET_LEFT_CHANNEL
							| AMP_SET_RIGHT_CHANNEL);
					hda_send_verbs(codec, &verb, NULL, 1);
				}

				dprintf("%s: Found input PIN (%s) connected to input CONV "
					"wid:%ld\n", __func__, kDefaultDevice[audioGroup->widgets[
						inputWidget - audioGroup->widget_start].d.pin.device],
					i + audioGroup->widget_start);
			}
		}
	}

	/* If we found any valid output channels, we're in the clear */
	if (audioGroup && audioGroup->playback_stream) {
		codec->audio_groups[codec->num_audio_groups++] = audioGroup;
		return B_OK;
	}

err:
	free(audioGroup);
	return status;
}


//	#pragma mark -


void
hda_codec_delete(hda_codec* codec)
{
	if (codec == NULL)
		return;

	delete_sem(codec->response_sem);

	for (uint32 i = 0; i < codec->num_audio_groups; i++) {
		hda_codec_delete_audio_group(codec->audio_groups[i]);
		codec->audio_groups[i] = NULL;
	}

	free(codec);
}


hda_codec*
hda_codec_new(hda_controller* controller, uint32 codecAddress)
{
	if (codecAddress > HDA_MAX_CODECS)
		return NULL;

	hda_codec* codec = (hda_codec*)calloc(1, sizeof(hda_codec));
	if (codec == NULL)
		return NULL;

	codec->controller = controller;
	codec->addr = codecAddress;
	codec->response_sem = create_sem(0, "hda_codec_response_sem");
	controller->codecs[codecAddress] = codec;

	struct {
		uint32 device : 16;
		uint32 vendor : 16;
		uint32 stepping : 8;
		uint32 revision : 8;
		uint32 minor : 4;
		uint32 major : 4;
		uint32 _reserved0 : 8;
		uint32 count : 8;
		uint32 _reserved1 : 8;
		uint32 start : 8;
		uint32 _reserved2 : 8;
	} response;
	corb_t verbs[3];

	verbs[0] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_VENDOR_ID);
	verbs[1] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_REVISION_ID);
	verbs[2] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER,
		PID_SUBORDINATE_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, (uint32*)&response, 3) != B_OK)
		goto err;

	dprintf("Codec %ld Vendor: %04lx Product: %04lx, Revision: "
		"%lu.%lu.%lu.%lu\n", codecAddress, response.vendor, response.device,
		response.major, response.minor, response.revision, response.stepping);

	for (uint32 nodeID = response.start; nodeID < response.start
			+ response.count; nodeID++) {
		uint32 groupType;
		verbs[0] = MAKE_VERB(codecAddress, nodeID, VID_GET_PARAMETER,
			PID_FUNCTION_GROUP_TYPE);

		if (hda_send_verbs(codec, verbs, &groupType, 1) != B_OK)
			goto err;

		if ((groupType & 0xff) == 1) {
			/* Found an Audio Function Group! */
			status_t status = hda_codec_new_audio_group(codec, nodeID);
			if (status != B_OK) {
				dprintf("hda: Failed to setup new audio function group (%s)!\n",
					strerror(status));
				goto err;
			}
		}
	}

	return codec;

err:
	controller->codecs[codecAddress] = NULL;
	hda_codec_delete(codec);
	return NULL;
}
