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
	corb_t verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAM,
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

	verbs[0] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAM, PID_STREAM_SUPPORT);
	verbs[1] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAM, PID_PCM_SUPPORT);

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

	verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAM, PID_OUTPUT_AMP_CAP);
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


static status_t
hda_codec_parse_audio_group(hda_audio_group* audioGroup)
{
	corb_t verbs[6];
	uint32 resp[6];
	uint32 widx;

	hda_widget_get_stream_support(audioGroup->codec, audioGroup->root_node_id,
		&audioGroup->supported_formats, &audioGroup->supported_rates);
	hda_widget_get_pm_support(audioGroup->codec, audioGroup->root_node_id,
		&audioGroup->supported_pm);

	verbs[0] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAM, PID_AUDIO_FG_CAP);
	verbs[1] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAM, PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(audioGroup->codec->addr, audioGroup->root_node_id,
		VID_GET_PARAM, PID_SUBORD_NODE_COUNT);

	if (hda_send_verbs(audioGroup->codec, verbs, resp, 3) != B_OK)
		return B_ERROR;

	dprintf("%s: Output delay: %ld samples, Input delay: %ld samples, "
		"Beep Generator: %s\n", __func__, resp[0] & 0xf,
		(resp[0] >> 8) & 0xf, (resp[0] & (1 << 16)) ? "yes" : "no");

	dprintf("%s: #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n",
		__func__, resp[4] & 0xFF, (resp[1] >> 8) & 0xFF,
		(resp[1] >> 16) & 0xFF, (resp[1] & (1 << 30)) ? "yes" : "no",
		(resp[1] & (1 << 31)) ? "yes" : "no");

	audioGroup->widget_start = resp[2] >> 16;
	audioGroup->widget_count = resp[2] & 0xFF;

	audioGroup->widgets = (hda_widget*)calloc(audioGroup->widget_count,
		sizeof(*audioGroup->widgets));
	if (audioGroup->widgets == NULL) {
		dprintf("ERROR: Not enough memory!\n");
		return B_NO_MEMORY;
	}

	/* Iterate over all Widgets and collect info */
	for (widx = 0; widx < audioGroup->widget_count; widx++) {
		uint32 wid = audioGroup->widget_start + widx;
		char buf[256];
		int off;

		verbs[0] = MAKE_VERB(audioGroup->codec->addr, wid, VID_GET_PARAM,
			PID_AUDIO_WIDGET_CAP);
		verbs[1] = MAKE_VERB(audioGroup->codec->addr, wid, VID_GET_PARAM,
			PID_CONNLIST_LEN);
		hda_send_verbs(audioGroup->codec, verbs, resp, 2);

		audioGroup->widgets[widx].type = (hda_widget_type)(resp[0] >> 20);
		audioGroup->widgets[widx].num_inputs = resp[1] & 0x7F;

		off = 0;
		if (resp[0] & (1 << 11))
			off += sprintf(buf + off, "[L-R Swap] ");

		if (resp[0] & (1 << 10)) {
			corb_t verb;
			uint32 resp;

			off += sprintf(buf+off, "[Power] ");

			/* We support power; switch us on! */
			verb = MAKE_VERB(audioGroup->codec->addr, wid,
				VID_SET_POWERSTATE, 0);
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

		switch (audioGroup->widgets[widx].type) {
			case WT_AUDIO_OUTPUT:
				dprintf("%ld:\tAudio Output\n", wid);
				hda_widget_get_stream_support(audioGroup->codec, wid,
					&audioGroup->widgets[widx].d.input.formats,
					&audioGroup->widgets[widx].d.input.rates);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, wid);
				break;
			case WT_AUDIO_INPUT:
				dprintf("%ld:\tAudio Input\n", wid);
				hda_widget_get_stream_support(audioGroup->codec, wid,
					&audioGroup->widgets[widx].d.input.formats,
					&audioGroup->widgets[widx].d.input.rates);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, wid);
				break;
			case WT_AUDIO_MIXER:
				dprintf("%ld:\tAudio Mixer\n", wid);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, wid);
				break;
			case WT_AUDIO_SELECTOR:
				dprintf("%ld:\tAudio Selector\n", wid);
				hda_widget_get_amplifier_capabilities(audioGroup->codec, wid);
				break;
			case WT_PIN_COMPLEX:
				dprintf("%ld:\tPin Complex\n", wid);
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, wid, VID_GET_PARAM,
					PID_PIN_CAP);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					audioGroup->widgets[widx].d.pin.input = resp[0] & (1 << 5);
					audioGroup->widgets[widx].d.pin.output = resp[0] & (1 << 4);

					dprintf("\t%s%s\n", 
						audioGroup->widgets[widx].d.pin.input ? "[Input] " : "",
						audioGroup->widgets[widx].d.pin.input ? "[Output]" : "");
				} else {
					dprintf("%s: Error getting Pin Complex IO\n", __func__);
				}

				verbs[0] = MAKE_VERB(audioGroup->codec->addr, wid,
					VID_GET_CFGDEFAULT, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					audioGroup->widgets[widx].d.pin.device = (pin_dev_type)
						((resp[0] >> 20) & 0xf);
					dprintf("\t%s, %s, %s, %s\n",
						kPortConnector[resp[0] >> 30],	
						kDefaultDevice[audioGroup->widgets[widx].d.pin.device],
						kConnectionType[(resp[0] >> 16) & 0xF],
						kJackColor[(resp[0] >> 12) & 0xF]);
				}

				hda_widget_get_amplifier_capabilities(audioGroup->codec, wid);
				break;
			case WT_POWER:
				dprintf("%ld:\tPower\n", wid);
				break;
			case WT_VOLUME_KNOB:
				dprintf("%ld:\tVolume Knob\n", wid);
				break;
			case WT_BEEP_GENERATOR:
				dprintf("%ld:\tBeep Generator\n", wid);
				break;
			case WT_VENDOR_DEFINED:
				dprintf("%ld:\tVendor Defined\n", wid);
				break;
			default:	/* Reserved */
				break;
		}

		dprintf("\t%s\n", buf);

		hda_widget_get_pm_support(audioGroup->codec, wid,
			&audioGroup->widgets[widx].pm);

		if (audioGroup->widgets[widx].num_inputs) {
			if (audioGroup->widgets[widx].num_inputs > 1) {
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, wid,
					VID_GET_CONNSEL, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK)
					audioGroup->widgets[widx].active_input = resp[0] & 0xFF;
				else
					audioGroup->widgets[widx].active_input = -1;
			} else 
				audioGroup->widgets[widx].active_input = -1;

			off = 0;

			for (uint32 i = 0; i < audioGroup->widgets[widx].num_inputs; i++) {
				if (!(i % 4)) {
					verbs[0] = MAKE_VERB(audioGroup->codec->addr, wid,
						VID_GET_CONNLENTRY, i);
					if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) != B_OK) {
						dprintf("%s: Error parsing inputs for widget %ld!\n",
							__func__, wid);
						break;
					}
				}

				if ((int32)i != audioGroup->widgets[widx].active_input) {
					off += sprintf(buf + off, "%ld ",
						(resp[0] >> (8 * (i % 4))) & 0xff);
				} else {
					off += sprintf(buf + off, "(%ld) ",
						(resp[0] >> (8 * (i % 4))) & 0xff);
				}

				audioGroup->widgets[widx].inputs[i]
					= (resp[0] >> (8 * (i % 4))) & 0xff;
			}

			dprintf("\t[ %s]\n", buf);
		}
	}

	return B_OK;
}


/*! Find path from 'wid' to a widget of type \a widgetType, returning its
	widget id.
	Returns 0 if not found.
*/
static uint32
hda_codec_audio_group_find_path(hda_audio_group* audioGroup, uint32 widget,
	hda_widget_type widgetType, uint32 depth)
{
	int widx = widget - audioGroup->widget_start;

	switch (audioGroup->widgets[widx].type) {
		case WT_AUDIO_MIXER:
			for (uint32 i = 0; i < audioGroup->widgets[widx].num_inputs; i++) {
				if (hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[widx].inputs[i], widgetType,
						depth + 1)) {
					if (audioGroup->widgets[widx].active_input == -1)
						audioGroup->widgets[widx].active_input = i;

					return audioGroup->widgets[widx].inputs[i];
				}
			}
			break;

		case WT_AUDIO_SELECTOR:
		{
			int32 i = audioGroup->widgets[widx].active_input;
			if (i != -1) {
				widget = audioGroup->widgets[widx].inputs[i];
				if (hda_codec_audio_group_find_path(audioGroup, widget,
						widgetType, depth + 1)) {
					return widget;
				}
			}
			break;
		}

		default:
			if (audioGroup->widgets[widx].type == widgetType)
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
			if (audioGroup->widgets[i].d.pin.device == PIN_DEV_HP_OUT
				|| audioGroup->widgets[i].d.pin.device == PIN_DEV_SPEAKER
				|| audioGroup->widgets[i].d.pin.device == PIN_DEV_LINE_OUT) {
				int32 inputIndex = audioGroup->widgets[i].active_input;
				if (inputIndex != -1) {
					outputWidget = hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[i].inputs[inputIndex],
						WT_AUDIO_OUTPUT, 0);
				} else {
					for (inputIndex = 0; (uint32)inputIndex
							< audioGroup->widgets[i].num_inputs; inputIndex++) {
						outputWidget = hda_codec_audio_group_find_path(audioGroup,
							audioGroup->widgets[i].inputs[inputIndex],
							WT_AUDIO_OUTPUT, 0);
						if (outputWidget) {
							corb_t verb = MAKE_VERB(codec->addr,
								i + audioGroup->widget_start, VID_SET_CONNSEL,
								inputIndex);
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
							VID_SET_AMPGAINMUTE,
							(1 << 15) | (1 << 13) | (1 << 12));
						verb[1] = MAKE_VERB(codec->addr,
							audioGroup->playback_stream->pin_widget,
							VID_SET_PINWCTRL,
							(1 << 7) | (1 << 6));
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
				for (inputIndex = 0; (uint32)inputIndex
						< audioGroup->widgets[i].num_inputs; inputIndex++) {
					inputWidget = hda_codec_audio_group_find_path(audioGroup,
						audioGroup->widgets[i].inputs[inputIndex],
						WT_PIN_COMPLEX, 0);
					if (inputWidget) {
						corb_t verb = MAKE_VERB(codec->addr,
							i + audioGroup->widget_start, VID_SET_CONNSEL,
							inputIndex);
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
						VID_SET_AMPGAINMUTE, (1 << 15) | (1 << 13) | (1 << 12));
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
	if (codec == NULL) {
		uint32 idx;

		delete_sem(codec->response_sem);

		for (idx = 0; idx < codec->num_audio_groups; idx++) {
			hda_codec_delete_audio_group(codec->audio_groups[idx]);
			codec->audio_groups[idx] = NULL;
		}

		free(codec);
	}
}


hda_codec*
hda_codec_new(hda_controller* controller, uint32 cad)
{
	hda_codec* codec = (hda_codec*)calloc(1, sizeof(hda_codec));
	uint32 responses[3];
	corb_t verbs[3];
	uint32 nodeID;

	if (codec == NULL)
		goto exit_new;

	codec->controller = controller;
	codec->addr = cad;
	codec->response_sem = create_sem(0, "hda_codec_response_sem");
	controller->codecs[cad] = codec;

	verbs[0] = MAKE_VERB(cad, 0, VID_GET_PARAM, PID_VENDORID);
	verbs[1] = MAKE_VERB(cad, 0, VID_GET_PARAM, PID_REVISIONID);
	verbs[2] = MAKE_VERB(cad, 0, VID_GET_PARAM, PID_SUBORD_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, responses, 3) != B_OK)
		goto cmd_failed;

	dprintf("Codec %ld Vendor: %04lx Product: %04lx\n",  
		cad, responses[0] >> 16, responses[0] & 0xffff);

	for (nodeID = responses[2] >> 16;
			nodeID < (responses[2] >> 16) + (responses[2] & 0xff); nodeID++) {
		uint32 resp;
		verbs[0] = MAKE_VERB(cad, nodeID, VID_GET_PARAM, PID_FUNCGRP_TYPE);

		if (hda_send_verbs(codec, verbs, &resp, 1) != B_OK)
			goto cmd_failed;

		if ((resp & 0xff) == 1) {
			/* Found an Audio Function Group! */
			status_t rc = hda_codec_new_audio_group(codec, nodeID);
			if (rc != B_OK) {
				dprintf("%s: Failed to setup new audio function group (%s)!\n",
					__func__, strerror(rc));
				goto cmd_failed;
			}
		}
	}

	goto exit_new;

cmd_failed:
	controller->codecs[cad] = NULL;
	hda_codec_delete(codec);
	codec = NULL;

exit_new:
	return codec;
}
