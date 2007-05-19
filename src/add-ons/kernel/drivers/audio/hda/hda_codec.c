#include "driver.h"
#include "hda_codec_defs.h"

const char* portcon[] = {
	"Jack", "None", "Fixed", "Dual"
};

const char* defdev[] = {
	"Line Out", "Speaker", "HP Out", "CD", "SPDIF out", "Digital Other Out", "Modem Line Side",
	"Modem Hand Side", "Line In", "AUX", "Mic In", "Telephony", "SPDIF In", "Digital Other In",
	"Reserved", "Other"
};

const char* conntype[] = {
	"N/A", "1/8\"", "1/4\"", "ATAPI internal", "RCA", "Optical", "Other Digital", "Other Analog",
	"Multichannel Analog (DIN)", "XLR/Professional", "RJ-11 (Modem)", "Combination", "-", "-", "-", "Other"
};

const char* jcolor[] = {
	"N/A", "Black", "Grey", "Blue", "Green", "Red", "Orange", "Yellow", "Purple", "Pink",
	"-", "-", "-", "-", "White", "Other"
};

static status_t
hda_widget_get_pm_support(hda_codec* codec, uint32 nid, uint32* pm)
{
	corb_t verb = MAKE_VERB(codec->addr,nid,VID_GET_PARAM,PID_POWERSTATE_SUPPORT);
	status_t rc;
	uint32 resp;
	
	if ((rc=hda_send_verbs(codec, &verb, &resp, 1)) == B_OK) {
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
hda_widget_get_stream_support(hda_codec* codec, uint32 nid, uint32* fmts, uint32* rates)
{
	corb_t verbs[2];
	uint32 resp[2];
	status_t rc;

	verbs[0] = MAKE_VERB(codec->addr,nid,VID_GET_PARAM,PID_STREAM_SUPPORT);
	verbs[1] = MAKE_VERB(codec->addr,nid,VID_GET_PARAM,PID_PCM_SUPPORT);
	if ((rc=hda_send_verbs(codec, verbs, resp, 2)) == B_OK) {
		*fmts = 0; *rates = 0;

		if (resp[2] & (1 << 0)) {
			if (resp[1] & (1 << 0))		*rates |= B_SR_8000;
			if (resp[1] & (1 << 1))		*rates |= B_SR_11025;
			if (resp[1] & (1 << 2))		*rates |= B_SR_16000;
			if (resp[1] & (1 << 3))		*rates |= B_SR_22050;
			if (resp[1] & (1 << 4))		*rates |= B_SR_32000;
			if (resp[1] & (1 << 5))		*rates |= B_SR_44100;
			if (resp[1] & (1 << 6))		*rates |= B_SR_48000;
			if (resp[1] & (1 << 7))		*rates |= B_SR_88200;
			if (resp[1] & (1 << 8))		*rates |= B_SR_96000;
			if (resp[1] & (1 << 9))		*rates |= B_SR_176400;
			if (resp[1] & (1 << 10))	*rates |= B_SR_192000;
			if (resp[1] & (1 << 11))	*rates |= B_SR_384000;
	
			if (resp[1] & (1<<16))		*fmts |= B_FMT_8BIT_S;
			if (resp[1] & (1<<17))		*fmts |= B_FMT_16BIT;
			if (resp[1] & (1<<18))		*fmts |= B_FMT_18BIT;
			if (resp[1] & (1<<19))		*fmts |= B_FMT_24BIT;
			if (resp[1] & (1<<20))		*fmts |= B_FMT_32BIT;
		}

		if (resp[0] & (1 << 1))			*fmts |= B_FMT_FLOAT;
		if (resp[0] & (1 << 2))			/* Sort out how to handle AC3 */;
	}
	
	return rc;
}

static status_t
hda_widget_get_amplifier_capabilities(hda_codec* codec, uint32 nid)
{
	status_t rc;
	corb_t verb;
	uint32 resp;
	
	verb = MAKE_VERB(codec->addr,nid,VID_GET_PARAM,PID_OUTPUT_AMP_CAP);
	if ((rc=hda_send_verbs(codec, &verb, &resp, 1)) == B_OK && resp != 0) {
		dprintf("\tAMP: Mute: %s, step size: %ld, # steps: %ld, offset: %ld\n",
			(resp & (1 << 31)) ? "supported" : "N/A",
			(resp >> 16) & 0x7F,
			(resp >> 8) & 0x7F,
			resp & 0x7F);
	}
	
	return rc;
}

static status_t
hda_codec_parse_afg(hda_afg* afg)
{
	corb_t verbs[6];
	uint32 resp[6];
	uint32 widx;

	hda_widget_get_stream_support(afg->codec, afg->root_nid, &afg->deffmts, &afg->defrates);
	hda_widget_get_pm_support(afg->codec, afg->root_nid, &afg->defpm);

	verbs[0] = MAKE_VERB(afg->codec->addr,afg->root_nid,VID_GET_PARAM,PID_AUDIO_FG_CAP);
	verbs[1] = MAKE_VERB(afg->codec->addr,afg->root_nid,VID_GET_PARAM,PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(afg->codec->addr,afg->root_nid,VID_GET_PARAM,PID_SUBORD_NODE_COUNT);

	if (hda_send_verbs(afg->codec, verbs, resp, 3) == B_OK) {
		dprintf("%s: Output delay: %ld samples, Input delay: %ld samples, Beep Generator: %s\n", __func__,
			resp[0] & 0xf, (resp[0] >> 8) & 0xf, (resp[0] & (1 << 16)) ? "yes" : "no");

		dprintf("%s: #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n", __func__,
			resp[4] & 0xFF, (resp[1] >> 8) & 0xFF, (resp[1] >> 16) & 0xFF,
			(resp[1] & (1 << 30)) ? "yes" : "no",
			(resp[1] & (1 << 31)) ? "yes" : "no");

		afg->wid_start = resp[2] >> 16;
		afg->wid_count = resp[2] & 0xFF;

		afg->widgets = calloc(afg->wid_count, sizeof(*afg->widgets));
		if (afg->widgets == NULL) {
			dprintf("ERROR: Not enough memory!\n");
			return B_NO_MEMORY;
		}

		/* Iterate over all Widgets and collect info */
		for (widx=0; widx < afg->wid_count; widx++) {
			uint32 wid = afg->wid_start + widx;
			char buf[256];
			int off;
			
			verbs[0] = MAKE_VERB(afg->codec->addr,wid,VID_GET_PARAM,PID_AUDIO_WIDGET_CAP);
			verbs[1] = MAKE_VERB(afg->codec->addr,wid,VID_GET_PARAM,PID_CONNLIST_LEN);
			hda_send_verbs(afg->codec, verbs, resp, 2);
			
			afg->widgets[widx].type = resp[0] >> 20;
			afg->widgets[widx].num_inputs = resp[1] & 0x7F;

			off = 0;
			if (resp[0] & (1 << 11)) off += sprintf(buf+off, "[L-R Swap] ");
			if (resp[0] & (1 << 10)) off += sprintf(buf+off, "[Power] ");
			if (resp[0] & (1 << 9)) off += sprintf(buf+off, "[Digital] ");
			if (resp[0] & (1 << 7)) off += sprintf(buf+off, "[Unsol Capable] ");
			if (resp[0] & (1 << 6)) off += sprintf(buf+off, "[Proc Widget] ");
			if (resp[0] & (1 << 5)) off += sprintf(buf+off, "[Stripe] ");
			if (resp[0] & (1 << 4)) off += sprintf(buf+off, "[Format Override] ");
			if (resp[0] & (1 << 3)) off += sprintf(buf+off, "[Amp Param Override] ");
			if (resp[0] & (1 << 2)) off += sprintf(buf+off, "[Out Amp] ");
			if (resp[0] & (1 << 1)) off += sprintf(buf+off, "[In Amp] ");
			if (resp[0] & (1 << 0)) off += sprintf(buf+off, "[Stereo] ");

			switch(afg->widgets[widx].type) {
				case WT_AUDIO_OUTPUT:
					dprintf("%ld:\tAudio Output\n", wid);
					hda_widget_get_stream_support(afg->codec, wid,
						&afg->widgets[widx].d.input.formats,
						&afg->widgets[widx].d.input.rates);
					hda_widget_get_amplifier_capabilities(afg->codec, wid);
					break;
				case WT_AUDIO_INPUT:
					dprintf("%ld:\tAudio Input\n", wid);
					hda_widget_get_stream_support(afg->codec, wid,
						&afg->widgets[widx].d.input.formats,
						&afg->widgets[widx].d.input.rates);
					hda_widget_get_amplifier_capabilities(afg->codec, wid);
					break;
				case WT_AUDIO_MIXER:
					dprintf("%ld:\tAudio Mixer\n", wid);
					hda_widget_get_amplifier_capabilities(afg->codec, wid);
					break;
				case WT_AUDIO_SELECTOR:
					dprintf("%ld:\tAudio Selector\n", wid);
					hda_widget_get_amplifier_capabilities(afg->codec, wid);
					break;
				case WT_PIN_COMPLEX:
					dprintf("%ld:\tPin Complex\n", wid);
					verbs[0] = MAKE_VERB(afg->codec->addr,wid,VID_GET_PARAM,PID_PIN_CAP);
					if (hda_send_verbs(afg->codec, verbs, resp, 1) == B_OK) {
						afg->widgets[widx].d.pin.input = resp[0] & (1 << 5);
						afg->widgets[widx].d.pin.output = resp[0] & (1 << 4);
						
						dprintf("\t%s%s\n", 
							afg->widgets[widx].d.pin.input ? "[Input] " : "",
							afg->widgets[widx].d.pin.input ? "[Output]" : "");
					} else {
						dprintf("%s: Error getting Pin Complex IO\n", __func__);
					}

					verbs[0] = MAKE_VERB(afg->codec->addr,wid,VID_GET_CFGDEFAULT,0);
					if (hda_send_verbs(afg->codec, verbs, resp, 1) == B_OK) {
						afg->widgets[widx].d.pin.device = (resp[0] >> 20) & 0xF;
						dprintf("\t%s, %s, %s, %s\n",
							portcon[resp[0] >> 30],	
							defdev[afg->widgets[widx].d.pin.device],
							conntype[(resp[0] >> 16) & 0xF],
							jcolor[(resp[0] >> 12) & 0xF]);
					}
					
					hda_widget_get_amplifier_capabilities(afg->codec, wid);
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

			hda_widget_get_pm_support(afg->codec, wid, &afg->widgets[widx].pm);
			
			if (afg->widgets[widx].num_inputs) {
				int idx;

				off = 0;

				if (afg->widgets[widx].num_inputs > 1) {
					verbs[0] = MAKE_VERB(afg->codec->addr,wid,VID_GET_CONNSEL,0);
					if (hda_send_verbs(afg->codec, verbs, resp, 1) == B_OK)
						afg->widgets[widx].active_input = resp[0] & 0xFF;
					else
						afg->widgets[widx].active_input = -1;
				} else 
					afg->widgets[widx].active_input = -1;
				
				for (idx=0; idx < afg->widgets[widx].num_inputs; idx ++) {
					if (!(idx % 4)) {
						verbs[0] = MAKE_VERB(afg->codec->addr,wid,VID_GET_CONNLENTRY,idx);
						if (hda_send_verbs(afg->codec, verbs, resp, 1) != B_OK) {
							dprintf("%s: Error parsing inputs for widget %ld!\n", __func__, wid);
							break;
						}
					}

					if (idx != afg->widgets[widx].active_input)
						off += sprintf(buf+off, "%ld ", (resp[0] >> (8*(idx%4))) & 0xFF);
					else
						off += sprintf(buf+off, "(%ld) ", (resp[0] >> (8*(idx%4))) & 0xFF);

					afg->widgets[widx].inputs[idx] = (resp[0] >> (8*(idx%4))) & 0xFF;
				}
				
				dprintf("\t[ %s]\n", buf);
			}
		}
	}
	
	return B_OK;
}

/* hda_codec_afg_find_path
 *
 * Find path from 'wid' to a widget of type 'wtype', returning its widget id.
 * Returns 0 if not found.
 */

static uint32
hda_codec_afg_find_path(hda_afg* afg, uint32 wid, uint32 wtype, uint32 depth)
{
	int widx = wid - afg->wid_start;
	int idx;

	switch(afg->widgets[widx].type) {
		case WT_AUDIO_MIXER:
			for (idx=0; idx < afg->widgets[widx].num_inputs; idx++) {
				if (hda_codec_afg_find_path(afg, afg->widgets[widx].inputs[idx], wtype, depth +1)) {
					if (afg->widgets[widx].active_input == -1)
						afg->widgets[widx].active_input = idx;

					return afg->widgets[widx].inputs[idx];
				}
			}
			break;

		case WT_AUDIO_SELECTOR:
			{
				int idx = afg->widgets[widx].active_input;
				if (idx != -1) {
					uint32 wid = afg->widgets[widx].inputs[idx];
					if (hda_codec_afg_find_path(afg, wid, wtype, depth +1)) {
						return wid;
					}
				}
			}
			break;

		default:
			if (afg->widgets[widx].type == wtype)
				return wid;

			break;
	}
	
	return 0;
}

static void
hda_afg_delete(hda_afg* afg)
{
	if (afg != NULL) {
		if (afg->playback_stream != NULL)
			hda_stream_delete(afg->playback_stream);

		if (afg->record_stream != NULL)
			hda_stream_delete(afg->record_stream);

		if (afg->widgets)
			free(afg->widgets);
			
		free(afg);
	}
}

static status_t
hda_codec_afg_new(hda_codec* codec, uint32 afg_nid)
{
	hda_afg* afg;
	status_t rc;
	uint32 idx;

	if ((afg=calloc(1, sizeof(hda_afg))) == NULL) {
		rc =  B_NO_MEMORY;
		goto done;
	}

	/* Setup minimal info needed by hda_codec_parse_afg */
	afg->root_nid = afg_nid;
	afg->codec = codec;

	/* Parse all widgets in Audio Function Group */
	rc = hda_codec_parse_afg(afg);
	if (rc != B_OK)
		goto free_afg;

	/* Setup for worst-case scenario;
		we cannot find any output Pin Widgets */
	rc = ENODEV;

	dprintf("%s: Scanning all %ld widgets for outputs/inputs....\n", 
		__func__, afg->wid_count);

	/* Try to locate all input/output channels */
	for (idx=0; idx < afg->wid_count; idx++) {
		uint32 output_wid = 0, input_wid = 0;
		int32 iidx;

		if (afg->widgets[idx].type == WT_PIN_COMPLEX && afg->widgets[idx].d.pin.output) {
			if 	(afg->widgets[idx].d.pin.device != PIN_DEV_HP_OUT &&
				afg->widgets[idx].d.pin.device != PIN_DEV_SPEAKER &&
				afg->widgets[idx].d.pin.device != PIN_DEV_LINE_OUT)
				continue;
	
			iidx = afg->widgets[idx].active_input;
			if (iidx != -1) {
				output_wid = hda_codec_afg_find_path(afg, afg->widgets[idx].inputs[iidx], WT_AUDIO_OUTPUT, 0);
			} else {
				for (iidx=0; iidx < afg->widgets[idx].num_inputs; iidx++) {
					output_wid = hda_codec_afg_find_path(afg, afg->widgets[idx].inputs[iidx], WT_AUDIO_OUTPUT, 0);
					if (output_wid) {
						corb_t verb = MAKE_VERB(codec->addr,idx+afg->wid_start,VID_SET_CONNSEL,iidx);
						if (hda_send_verbs(codec, &verb, NULL, 1) != B_OK)
							dprintf("%s: Setting output selector failed!\n", __func__);
						break;
					}
				}
			}
	
			if (output_wid) {
				if (!afg->playback_stream) {
					corb_t verb;
	
					/* Setup playback/record streams for Multi Audio API */
					afg->playback_stream = hda_stream_new(afg->codec->ctrlr, STRM_PLAYBACK);
					afg->record_stream = hda_stream_new(afg->codec->ctrlr, STRM_RECORD);
		
					afg->playback_stream->pin_wid = idx + afg->wid_start;
					afg->playback_stream->io_wid = output_wid;
		
					/* FIXME: Force Pin Widget to unmute */
					verb = MAKE_VERB(codec->addr, afg->playback_stream->pin_wid,
						VID_SET_AMPGAINMUTE, (1 << 15) | (1 << 13) | (1 << 12));
					hda_send_verbs(codec, &verb, NULL, 1);
				}	
	
				dprintf("%s: Found output PIN (%s) connected to output CONV wid:%ld\n", 
					__func__, defdev[afg->widgets[idx].d.pin.device], output_wid);
			}
		} 

		if (afg->widgets[idx].type == WT_AUDIO_INPUT) {
			iidx = afg->widgets[idx].active_input;
			if (iidx != -1) {
				input_wid = hda_codec_afg_find_path(afg, afg->widgets[idx].inputs[iidx], WT_PIN_COMPLEX, 0);
			} else {
				for (iidx=0; iidx < afg->widgets[idx].num_inputs; iidx++) {
					input_wid = hda_codec_afg_find_path(afg, afg->widgets[idx].inputs[iidx], WT_PIN_COMPLEX, 0);
					if (input_wid) {
						corb_t verb = MAKE_VERB(codec->addr,idx+afg->wid_start,VID_SET_CONNSEL,iidx);
						if (hda_send_verbs(codec, &verb, NULL, 1) != B_OK)
							dprintf("%s: Setting input selector failed!\n", __func__);
						break;
					}
				}
			}
	
			if (input_wid) {
				if (!afg->record_stream) {
					corb_t verb;
	
					/* Setup playback/record streams for Multi Audio API */
					afg->record_stream = hda_stream_new(afg->codec->ctrlr, STRM_RECORD);
		
					afg->record_stream->pin_wid = input_wid;
					afg->record_stream->io_wid = idx + afg->wid_start;
		
					/* FIXME: Force Pin Widget to unmute */
					verb = MAKE_VERB(codec->addr, afg->record_stream->pin_wid,
						VID_SET_AMPGAINMUTE, (1 << 15) | (1 << 13) | (1 << 12));
					hda_send_verbs(codec, &verb, NULL, 1);
				}	
	
				dprintf("%s: Found input PIN (%s) connected to input CONV wid:%ld\n", 
					__func__, defdev[afg->widgets[input_wid-afg->wid_start].d.pin.device], idx+afg->wid_start);
			}
		}
	}

	/* If we found any valid output channels, we're in the clear */
	if (afg && afg->playback_stream) {
		codec->afgs[codec->num_afgs++] = afg;
		rc = B_OK;
		goto done;
	}

free_afg:
	free(afg);

done:
	return rc;
}

void
hda_codec_delete(hda_codec* codec)
{
	if (codec != NULL) {
		uint32 idx;
		
		delete_sem(codec->response_sem);

		for (idx=0; idx < codec->num_afgs; idx++) {
			hda_afg_delete(codec->afgs[idx]);
			codec->afgs[idx] = NULL;
		}
		
		free(codec);
	}
}

hda_codec*
hda_codec_new(hda_controller* ctrlr, uint32 cad)
{
	hda_codec* codec = calloc(1, sizeof(hda_codec));
	uint32 responses[3];
	corb_t verbs[3];
	status_t rc;
	uint32 nid;

	if (codec == NULL) goto exit_new;

	codec->ctrlr = ctrlr;
	codec->addr = cad;
	codec->response_sem = create_sem(0, "hda_codec_response_sem");
	ctrlr->codecs[cad] = codec;
	
	verbs[0] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_VENDORID);
	verbs[1] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_REVISIONID);
	verbs[2] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_SUBORD_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, responses, 3) != B_OK)
		goto cmd_failed;

	dprintf("Codec %ld Vendor: %04lx Product: %04lx\n",  
		cad, responses[0] >> 16, responses[0] & 0xFFFF);
	
	for (nid=responses[2] >> 16;
		nid < (responses[2] >> 16) + (responses[2] & 0xFF); 
		nid++) {
		uint32 resp;
		verbs[0] = MAKE_VERB(cad,nid,VID_GET_PARAM,PID_FUNCGRP_TYPE);
				
		if ((rc=hda_send_verbs(codec, verbs, &resp, 1)) != B_OK)
			goto cmd_failed;

		if ((resp&0xFF) == 1) {
			/* Found an Audio Function Group! */
			if ((rc=hda_codec_afg_new(codec, nid)) != B_OK) {
				dprintf("%s: Failed to setup new audio function group (%s)!\n",
					__func__, strerror(rc));
				goto cmd_failed;
			}
		}
	}

	goto exit_new;

cmd_failed:
	ctrlr->codecs[cad] = NULL;
	hda_codec_delete(codec);
	codec = NULL;

exit_new:
	return codec;
}
