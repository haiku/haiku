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
hda_codec_parse_afg(hda_codec* codec, uint32 afg_nid)
{
	corb_t verbs[6];
	uint32 resp[6];
	uint32 widx;

	hda_widget_get_stream_support(codec, afg_nid, &codec->afg_deffmts, &codec->afg_defrates);
	hda_widget_get_pm_support(codec, afg_nid, &codec->afg_defpm);

	verbs[0] = MAKE_VERB(codec->addr,afg_nid,VID_GET_PARAM,PID_AUDIO_FG_CAP);
	verbs[1] = MAKE_VERB(codec->addr,afg_nid,VID_GET_PARAM,PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(codec->addr,afg_nid,VID_GET_PARAM,PID_SUBORD_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, resp, 3) == B_OK) {
		dprintf("%s: Output delay: %ld samples, Input delay: %ld samples, Beep Generator: %s\n", __func__,
			resp[0] & 0xf, (resp[0] >> 8) & 0xf, (resp[0] & (1 << 16)) ? "yes" : "no");

		dprintf("%s: #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n", __func__,
			resp[4] & 0xFF, (resp[1] >> 8) & 0xFF, (resp[1] >> 16) & 0xFF,
			(resp[1] & (1 << 30)) ? "yes" : "no",
			(resp[1] & (1 << 31)) ? "yes" : "no");

		codec->afg_wid_start = resp[2] >> 16;
		codec->afg_wid_count = resp[2] & 0xFF;

		codec->afg_widgets = calloc(sizeof(*codec->afg_widgets), codec->afg_wid_count);
		if (codec->afg_widgets == NULL) {
			dprintf("ERROR: Not enough memory!\n");
			return B_NO_MEMORY;
		}

		/* Only now mark the AFG as found and used */
		codec->afg_nid = afg_nid;

		/* Iterate over all Widgets and collect info */
		for (widx=0; widx < codec->afg_wid_count; widx++) {
			uint32 wid = codec->afg_wid_start + widx;
			char buf[256];
			int off;
			
			verbs[0] = MAKE_VERB(codec->addr,wid,VID_GET_PARAM,PID_AUDIO_WIDGET_CAP);
			verbs[1] = MAKE_VERB(codec->addr,wid,VID_GET_PARAM,PID_CONNLIST_LEN);
			hda_send_verbs(codec, verbs, resp, 2);
			
			codec->afg_widgets[widx].type = resp[0] >> 20;
			codec->afg_widgets[widx].num_inputs = resp[1] & 0x7F;

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

			switch(codec->afg_widgets[widx].type) {
				case WT_AUDIO_OUTPUT:
					dprintf("%ld:\tAudio Output\n", wid);
					hda_widget_get_stream_support(codec, wid,
						&codec->afg_widgets[widx].d.input.formats,
						&codec->afg_widgets[widx].d.input.rates);
					hda_widget_get_amplifier_capabilities(codec, wid);
					break;
				case WT_AUDIO_INPUT:
					dprintf("%ld:\tAudio Input\n", wid);
					hda_widget_get_stream_support(codec, wid,
						&codec->afg_widgets[widx].d.input.formats,
						&codec->afg_widgets[widx].d.input.rates);
					hda_widget_get_amplifier_capabilities(codec, wid);
					break;
				case WT_AUDIO_MIXER:
					dprintf("%ld:\tAudio Mixer\n", wid);
					hda_widget_get_amplifier_capabilities(codec, wid);
					break;
				case WT_AUDIO_SELECTOR:
					dprintf("%ld:\tAudio Selector\n", wid);
					hda_widget_get_amplifier_capabilities(codec, wid);
					break;
				case WT_PIN_COMPLEX:
					dprintf("%ld:\tPin Complex\n", wid);
					verbs[0] = MAKE_VERB(codec->addr,wid,VID_GET_PARAM,PID_PIN_CAP);
					if (hda_send_verbs(codec, verbs, resp, 1) == B_OK) {
						codec->afg_widgets[widx].d.pin.input = resp[0] & (1 << 5);
						codec->afg_widgets[widx].d.pin.output = resp[0] & (1 << 4);
					}

					verbs[0] = MAKE_VERB(codec->addr,wid,VID_GET_CFGDEFAULT,0);
					if (hda_send_verbs(codec, verbs, resp, 1) == B_OK) {
						codec->afg_widgets[widx].d.pin.device = (resp[0] >> 20) & 0xF;
						dprintf("\t%s, %s, %s, %s\n",
							portcon[resp[0] >> 30],	
							defdev[codec->afg_widgets[widx].d.pin.device],
							conntype[(resp[0] >> 16) & 0xF],
							jcolor[(resp[0] >> 12) & 0xF]);
					}
					
					hda_widget_get_amplifier_capabilities(codec, wid);
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

			hda_widget_get_pm_support(codec, wid, &codec->afg_widgets[widx].pm);
			
			if (codec->afg_widgets[widx].num_inputs) {
				int idx;

				off = 0;

				if (codec->afg_widgets[widx].num_inputs > 1) {
					verbs[0] = MAKE_VERB(codec->addr,wid,VID_GET_CONNSEL,0);
					if (hda_send_verbs(codec, verbs, resp, 1) == B_OK)
						codec->afg_widgets[widx].active_input = resp[0] & 0xFF;
					else
						codec->afg_widgets[widx].active_input = -1;
				} else 
					codec->afg_widgets[widx].active_input = -1;
				
				for (idx=0; idx < codec->afg_widgets[widx].num_inputs; idx ++) {
					if (!(idx % 4)) {
						verbs[0] = MAKE_VERB(codec->addr,wid,VID_GET_CONNLENTRY,idx);
						if (hda_send_verbs(codec, verbs, resp, 1) != B_OK) {
							dprintf("%s: Error parsing inputs for widget %ld!\n", __func__, wid);
							break;
						}
					}

					if (idx != codec->afg_widgets[widx].active_input)
						off += sprintf(buf+off, "%ld ", (resp[0] >> (8*(idx%4))) & 0xFF);
					else
						off += sprintf(buf+off, "(%ld) ", (resp[0] >> (8*(idx%4))) & 0xFF);

					codec->afg_widgets[widx].inputs[idx] = (resp[0] >> (8*(idx%4))) & 0xFF;
				}
				
				dprintf("\t[ %s]\n", buf);
			}
		}
	}
	
	return B_OK;
}

static uint32
hda_codec_afg_find_dac_path(hda_codec* codec, uint32 wid, uint32 depth)
{
	int widx = wid - codec->afg_wid_start;
	int idx;

	switch(codec->afg_widgets[widx].type) {
		case WT_AUDIO_OUTPUT:
			return wid;

		case WT_AUDIO_MIXER:
			for (idx=0; idx < codec->afg_widgets[widx].num_inputs; idx++) {
				if (hda_codec_afg_find_dac_path(codec, codec->afg_widgets[widx].inputs[idx], depth +1)) {
					if (codec->afg_widgets[widx].active_input == -1)
						codec->afg_widgets[widx].active_input = idx;

					return codec->afg_widgets[widx].inputs[idx];
				}
			}
			break;

		case WT_AUDIO_SELECTOR:
			{
				int idx = codec->afg_widgets[widx].active_input;
				if (idx != -1) {
					uint32 wid = codec->afg_widgets[widx].inputs[idx];
					if (hda_codec_afg_find_dac_path(codec, wid, depth +1)) {
						return wid;
					}
				}
			}
			break;

		default:
			break;
	}
	
	return 0;
}

static void
hda_codec_audiofg_new(hda_codec* codec, uint32 afg_nid)
{
	uint32 idx;
						
	/* FIXME: Bail if this isn't the first Audio Function Group we find... */
	if (codec->afg_nid != 0) {
		dprintf("SORRY: This driver currently only supports a single Audio Function Group!\n");
		return;
	}

	/* Parse all widgets in Audio Function Group */
	hda_codec_parse_afg(codec, afg_nid);

	/* Try to locate all output channels */
	for (idx=0; idx < codec->afg_wid_count; idx++) {
		uint32 iidx, output_wid = 0;

		if (codec->afg_widgets[idx].type != WT_PIN_COMPLEX)
			continue;
		if (codec->afg_widgets[idx].d.pin.output)
			continue;
		if 	(codec->afg_widgets[idx].d.pin.device != PIN_DEV_HP_OUT &&
			codec->afg_widgets[idx].d.pin.device != PIN_DEV_SPEAKER &&
			codec->afg_widgets[idx].d.pin.device != PIN_DEV_LINE_OUT)
			continue;

		iidx = codec->afg_widgets[idx].active_input;
		if (iidx != -1) {
			output_wid = hda_codec_afg_find_dac_path(codec, codec->afg_widgets[idx].inputs[iidx], 0);
		} else {
			for (iidx=0; iidx < codec->afg_widgets[idx].num_inputs; iidx++) {
				output_wid = hda_codec_afg_find_dac_path(codec, codec->afg_widgets[idx].inputs[iidx], 0);
				if (output_wid) {
					corb_t verb = MAKE_VERB(codec->addr,idx+codec->afg_wid_start,VID_SET_CONNSEL,iidx);
					if (hda_send_verbs(codec, &verb, NULL, 1) != B_OK)
						dprintf("%s: Setting output selector failed!\n", __func__);
					break;
				}
			}
		}

		if (output_wid) {
			corb_t verb;

			codec->playback_stream->pin_wid = idx + codec->afg_wid_start;
			codec->playback_stream->io_wid = output_wid;

			dprintf("%s: Found output PIN (%s) connected to output CONV wid:%ld\n", 
				__func__, defdev[codec->afg_widgets[idx].d.pin.device], output_wid);

			/* FIXME: Force Pin Widget to unmute */
			verb = MAKE_VERB(codec->addr, codec->playback_stream->pin_wid,
				VID_SET_AMPGAINMUTE, (1 << 15) | (1 << 13) | (1 << 12));
			hda_send_verbs(codec, &verb, NULL, 1);
			break;
		}
	}
}

hda_codec*
hda_codec_new(hda_controller* ctrlr, uint32 cad)
{
	hda_codec* codec = calloc(sizeof(hda_codec),1);
	if (codec) {
		uint32 responses[3];
		corb_t verbs[3];
		status_t rc;
		uint32 nid;
	
		codec->ctrlr = ctrlr;
		codec->addr = cad;
		codec->response_count = 0;
		codec->response_sem = create_sem(0, "hda_codec_response_sem");
		codec->afg_nid = 0;
		ctrlr->codecs[cad] = codec;
	
		/* Setup playback/record streams for Multi Audio API */
		codec->playback_stream = hda_stream_alloc(ctrlr, STRM_PLAYBACK);
		codec->record_stream = hda_stream_alloc(ctrlr, STRM_RECORD);

		verbs[0] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_VENDORID);
		verbs[1] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_REVISIONID);
		verbs[2] = MAKE_VERB(cad,0,VID_GET_PARAM,PID_SUBORD_NODE_COUNT);

		if (hda_send_verbs(codec, verbs, responses, 3) == B_OK) {
			dprintf("Codec %ld Vendor: %04lx Product: %04lx\n",  
				cad, responses[0] >> 16, responses[0] & 0xFFFF);
	
			for (nid=responses[2] >> 16;
				nid < (responses[2] >> 16) + (responses[2] & 0xFF); 
				nid++) {
				uint32 resp;
				verbs[0] = MAKE_VERB(cad,nid,VID_GET_PARAM,PID_FUNCGRP_TYPE);
				
				if ((rc=hda_send_verbs(codec, verbs, &resp, 1)) == B_OK && (resp&0xFF) == 1) {
					/* Found an Audio Function Group! */
					hda_codec_audiofg_new(codec, nid);
				} else
					dprintf("%s: FG %ld: %s\n", __func__, nid, strerror(rc));
			}	
		}
	}

	return codec;
}
