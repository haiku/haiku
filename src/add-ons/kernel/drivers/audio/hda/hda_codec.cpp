/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, korli@users.berlios.de
 */


#include "driver.h"
#include "hda_codec_defs.h"

#undef TRACE
#define TRACE_CODEC
#ifdef TRACE_CODEC
#define TRACE(a...) dprintf(a)
#else
#define TRACE(a...)
#endif
#define ERROR(a...) dprintf(a)

#define HDA_ALL 0xffffffff
#define HDA_QUIRK_GPIO_COUNT	8
#define HDA_QUIRK_GPIO0		(1 << 0)
#define HDA_QUIRK_GPIO1		(1 << 1)
#define HDA_QUIRK_GPIO2		(1 << 2)
#define HDA_QUIRK_GPIO3		(1 << 3)
#define HDA_QUIRK_GPIO4		(1 << 4)
#define HDA_QUIRK_GPIO5		(1 << 5)
#define HDA_QUIRK_GPIO6		(1 << 6)
#define HDA_QUIRK_GPIO7		(1 << 7)
#define HDA_QUIRK_IVREF50	(1 << 8)
#define HDA_QUIRK_IVREF80	(1 << 9)
#define HDA_QUIRK_IVREF100	(1 << 10)
#define HDA_QUIRK_OVREF50	(1 << 11)
#define HDA_QUIRK_OVREF80	(1 << 12)
#define HDA_QUIRK_OVREF100	(1 << 13)
#define HDA_QUIRK_IVREF (HDA_QUIRK_IVREF50 | HDA_QUIRK_IVREF80 \
	| HDA_QUIRK_IVREF100)
#define HDA_QUIRK_OVREF (HDA_QUIRK_OVREF50 | HDA_QUIRK_OVREF80 \
	| HDA_QUIRK_OVREF100)


#define ANALOGDEVICES_VENDORID		0x11d4
#define CIRRUSLOGIC_VENDORID		0x1013
#define CONEXANT_VENDORID			0x14f1
#define IDT_VENDORID				0x111d
#define REALTEK_VENDORID			0x10ec
#define SIGMATEL_VENDORID			0x8384


static const char* kPortConnector[] = {
	"Jack", "None", "Fixed", "Dual"
};

static const char* kDefaultDevice[] = {
	"Line out", "Speaker", "HP out", "CD", "SPDIF out", "Digital other out",
	"Modem line side", "Modem hand side", "Line in", "AUX", "Mic in",
	"Telephony", "SPDIF in", "Digital other in", "Reserved", "Other"
};

static const char* kConnectionType[] = {
	"N/A", "1/8\"", "1/4\"", "ATAPI internal", "RCA", "Optical",
	"Other digital", "Other analog", "Multichannel analog (DIN)",
	"XLR/Professional", "RJ-11 (modem)", "Combination", "-", "-", "-", "Other"
};

static const char* kJackColor[] = {
	"N/A", "Black", "Grey", "Blue", "Green", "Red", "Orange", "Yellow",
	"Purple", "Pink", "-", "-", "-", "-", "White", "Other"
};


static const char*
get_widget_type_name(hda_widget_type type)
{
	switch (type) {
		case WT_AUDIO_OUTPUT:
			return "Audio output";
		case WT_AUDIO_INPUT:
			return "Audio input";
		case WT_AUDIO_MIXER:
			return "Audio mixer";
		case WT_AUDIO_SELECTOR:
			return "Audio selector";
		case WT_PIN_COMPLEX:
			return "Pin complex";
		case WT_POWER:
			return "Power";
		case WT_VOLUME_KNOB:
			return "Volume knob";
		case WT_BEEP_GENERATOR:
			return "Beep generator";
		case WT_VENDOR_DEFINED:
			return "Vendor defined";
		default:
			return "Unknown";
	}
}


const char*
get_widget_location(uint32 location)
{
	switch (location >> 4) {
		case 0:
		case 2:
			switch (location & 0xf) {
				case 2:
					return "Front";
				case 3:
					return "Left";
				case 4:
					return "Right";
				case 5:
					return "Top";
				case 6:
					return "Bottom";
				case 7:
					return "Rear panel";
				case 8:
					return "Drive bay";
				case 0:
				case 1:
				default:
					return NULL;
			}
		case 1:
			switch (location & 0xf) {
				case 7:
					return "Riser";
				case 8:
					return "HDMI";
				default:
					return NULL;
			}
		case 3:
			switch (location & 0xf) {
				case 6:
					return "Bottom";
				case 7:
					return "Inside lid";
				case 8:
					return "Outside lid";
				default:
					return NULL;
			}
	}
	return NULL;
}


static void
dump_widget_audio_capabilities(uint32 capabilities)
{
	static const struct {
		uint32		flag;
		const char*	name;
	} kFlags[] = {
		{AUDIO_CAP_CP_CAPS, "CP caps"},
		{AUDIO_CAP_LEFT_RIGHT_SWAP, "L-R swap"},
		{AUDIO_CAP_POWER_CONTROL, "Power"},
		{AUDIO_CAP_DIGITAL, "Digital"},
		{AUDIO_CAP_CONNECTION_LIST, "Conn. list"},
		{AUDIO_CAP_UNSOLICITED_RESPONSES, "Unsol. responses"},
		{AUDIO_CAP_PROCESSING_CONTROLS, "Proc widget"},
		{AUDIO_CAP_STRIPE, "Stripe"},
		{AUDIO_CAP_FORMAT_OVERRIDE, "Format override"},
		{AUDIO_CAP_AMPLIFIER_OVERRIDE, "Amplifier override"},
		{AUDIO_CAP_OUTPUT_AMPLIFIER, "Out amplifier"},
		{AUDIO_CAP_INPUT_AMPLIFIER, "In amplifier"},
		{AUDIO_CAP_STEREO, "Stereo"},
	};

	char buffer[256];
	int offset = 0;

	for (uint32 j = 0; j < sizeof(kFlags) / sizeof(kFlags[0]); j++) {
		if (capabilities & kFlags[j].flag)
			offset += sprintf(buffer + offset, "[%s] ", kFlags[j].name);
	}

	if (offset != 0)
		TRACE("\t%s\n", buffer);
}


static void
dump_widget_inputs(hda_widget& widget)
{
	// dump connections

	char buffer[256];
	int offset = 0;

	for (uint32 i = 0; i < widget.num_inputs; i++) {
		int32 input = widget.inputs[i];

		if ((int32)i != widget.active_input)
			offset += sprintf(buffer + offset, "%ld ", input);
		else
			offset += sprintf(buffer + offset, "<%ld> ", input);
	}

	if (offset != 0)
		TRACE("\tInputs: %s\n", buffer);
}


static void
dump_widget_amplifier_capabilities(hda_widget& widget, bool input)
{
	uint32 capabilities;
	if (input)
		capabilities = widget.capabilities.input_amplifier;
	else
		capabilities = widget.capabilities.output_amplifier;

	if (capabilities == 0)
		return;

	TRACE("\t%s Amp: %sstep size: %f dB, # steps: %ld, offset to 0 dB: "
		"%ld\n", input ? "In" : "Out",
		(capabilities & AMP_CAP_MUTE) != 0 ? "supports mute, " : "",
		AMP_CAP_STEP_SIZE(capabilities),
		AMP_CAP_NUM_STEPS(capabilities),
		AMP_CAP_OFFSET(capabilities));
}


static void
dump_widget_pm_support(hda_widget& widget)
{
	TRACE("\tSupported power states: %s%s%s%s%s%s%s%s\n",
		widget.pm & POWER_STATE_D0 ? "D0 " : "",
		widget.pm & POWER_STATE_D1 ? "D1 " : "",
		widget.pm & POWER_STATE_D2 ? "D2 " : "",
		widget.pm & POWER_STATE_D3 ? "D3 " : "",
		widget.pm & POWER_STATE_D3COLD ? "D3COLD " : "",
		widget.pm & POWER_STATE_S3D3COLD ? "S3D3COLD " : "",
		widget.pm & POWER_STATE_CLKSTOP ? "CLKSTOP " : "",
		widget.pm & POWER_STATE_EPSS ? "EPSS " : "");
}


static void
dump_widget_stream_support(hda_widget& widget)
{
	TRACE("\tSupported formats: %s%s%s%s%s%s%s%s%s\n",
		widget.d.io.formats & B_FMT_8BIT_S ? "8bits " : "",
		widget.d.io.formats & B_FMT_16BIT ? "16bits " : "",
		widget.d.io.formats & B_FMT_20BIT ? "20bits " : "",
		widget.d.io.formats & B_FMT_24BIT ? "24bits " : "",
		widget.d.io.formats & B_FMT_32BIT ? "32bits " : "",
		widget.d.io.formats & B_FMT_FLOAT ? "float " : "",
		widget.d.io.formats & B_FMT_DOUBLE ? "double " : "",
		widget.d.io.formats & B_FMT_EXTENDED ? "extended " : "",
		widget.d.io.formats & B_FMT_BITSTREAM ? "bitstream " : "");
	TRACE("\tSupported rates: %s%s%s%s%s%s%s%s%s%s%s%s\n",
		widget.d.io.rates & B_SR_8000 ? "8khz " : "",
		widget.d.io.rates & B_SR_11025 ? "11khz " : "",
		widget.d.io.rates & B_SR_16000 ? "16khz " : "",
		widget.d.io.rates & B_SR_22050 ? "22khz " : "",
		widget.d.io.rates & B_SR_32000 ? "32khz " : "",
		widget.d.io.rates & B_SR_44100 ? "44khz " : "",
		widget.d.io.rates & B_SR_48000 ? "48khz " : "",
		widget.d.io.rates & B_SR_88200 ? "88khz " : "",
		widget.d.io.rates & B_SR_96000 ? "96khz " : "",
		widget.d.io.rates & B_SR_176400 ? "176khz " : "",
		widget.d.io.rates & B_SR_192000 ? "192khz " : "",
		widget.d.io.rates & B_SR_384000 ? "384khz " : "");

}


static void
dump_pin_complex_capabilities(hda_widget& widget)
{
	TRACE("\t%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		widget.d.pin.capabilities & PIN_CAP_IMP_SENSE ? "[Imp Sense] " : "",
		widget.d.pin.capabilities & PIN_CAP_TRIGGER_REQ ? "[Trigger Req]" : "",
		widget.d.pin.capabilities & PIN_CAP_PRES_DETECT ? "[Pres Detect]" : "",
		widget.d.pin.capabilities & PIN_CAP_HP_DRIVE ? "[HP Drive]" : "",
		widget.d.pin.capabilities & PIN_CAP_OUTPUT ? "[Output]" : "",
		widget.d.pin.capabilities & PIN_CAP_INPUT ? "[Input]" : "",
		widget.d.pin.capabilities & PIN_CAP_BALANCE ? "[Balance]" : "",
		widget.d.pin.capabilities & PIN_CAP_VREF_CTRL_HIZ ? "[VRef HIZ]" : "",
		widget.d.pin.capabilities & PIN_CAP_VREF_CTRL_50 ? "[VRef 50]" : "",
		widget.d.pin.capabilities & PIN_CAP_VREF_CTRL_GROUND ? "[VRef Gr]" : "",
		widget.d.pin.capabilities & PIN_CAP_VREF_CTRL_80 ? "[VRef 80]" : "",
		widget.d.pin.capabilities & PIN_CAP_VREF_CTRL_100 ? "[VRef 100]" : "",
		widget.d.pin.capabilities & PIN_CAP_EAPD_CAP ? "[EAPD]" : "");
}


static void
dump_audiogroup_widgets(hda_audio_group* audioGroup)
{
	TRACE("\tAudiogroup:\n");
	/* Iterate over all Widgets and collect info */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];
		uint32 nodeID = audioGroup->widget_start + i;

		TRACE("%ld: %s\n", nodeID, get_widget_type_name(widget.type));

		switch (widget.type) {
			case WT_AUDIO_OUTPUT:
			case WT_AUDIO_INPUT:
				break;

			case WT_PIN_COMPLEX:
				dump_pin_complex_capabilities(widget);
				break;

			default:
				break;
		}

		dump_widget_pm_support(widget);
		dump_widget_audio_capabilities(widget.capabilities.audio);
		dump_widget_amplifier_capabilities(widget, true);
		dump_widget_amplifier_capabilities(widget, false);
		dump_widget_inputs(widget);
	}
}


//	#pragma mark -


static status_t
hda_get_pm_support(hda_codec* codec, uint32 nodeID, uint32* pm)
{
	corb_t verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_POWERSTATE_SUPPORT);
	status_t rc;
	uint32 response;

	if ((rc = hda_send_verbs(codec, &verb, &response, 1)) == B_OK) {
		*pm = response & 0xf;
	}

	return rc;
}


static status_t
hda_get_stream_support(hda_codec* codec, uint32 nodeID, uint32* formats,
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
	if ((resp[0] & STREAM_AC3) != 0) {
		*formats |= B_FMT_BITSTREAM;
	}

	return B_OK;
}


//	#pragma mark - widget functions


static status_t
hda_widget_get_pm_support(hda_audio_group* audioGroup, hda_widget* widget)
{
	return hda_get_pm_support(audioGroup->codec, widget->node_id,
		&widget->pm);
}


static status_t
hda_widget_get_stream_support(hda_audio_group* audioGroup, hda_widget* widget)
{
	if (audioGroup->widget.node_id != widget->node_id
		&& (widget->capabilities.audio & AUDIO_CAP_FORMAT_OVERRIDE) == 0) {
		// adopt capabilities of the audio group
		widget->d.io.formats = audioGroup->widget.d.io.formats;
		widget->d.io.rates = audioGroup->widget.d.io.rates;
		return B_OK;
	}

	return hda_get_stream_support(audioGroup->codec, widget->node_id,
		&widget->d.io.formats, &widget->d.io.rates);
}


static status_t
hda_widget_get_amplifier_capabilities(hda_audio_group* audioGroup,
	hda_widget* widget)
{
	uint32 response;
	corb_t verb;

	if ((widget->capabilities.audio & AUDIO_CAP_OUTPUT_AMPLIFIER) != 0
		|| audioGroup->widget.node_id == widget->node_id) {
		if ((widget->capabilities.audio & AUDIO_CAP_AMPLIFIER_OVERRIDE) != 0
			|| audioGroup->widget.node_id == widget->node_id) {
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_PARAMETER, PID_OUTPUT_AMPLIFIER_CAP);
			status_t status = hda_send_verbs(audioGroup->codec, &verb,
				&response, 1);
			if (status < B_OK)
				return status;

			widget->capabilities.output_amplifier = response;
		} else {
			// adopt capabilities from the audio function group
			widget->capabilities.output_amplifier
				= audioGroup->widget.capabilities.output_amplifier;
		}
	}

	if ((widget->capabilities.audio & AUDIO_CAP_INPUT_AMPLIFIER) != 0
		|| audioGroup->widget.node_id == widget->node_id) {
		if ((widget->capabilities.audio & AUDIO_CAP_AMPLIFIER_OVERRIDE
			|| audioGroup->widget.node_id == widget->node_id) != 0) {
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_PARAMETER, PID_INPUT_AMPLIFIER_CAP);
			status_t status = hda_send_verbs(audioGroup->codec, &verb,
				&response, 1);
			if (status < B_OK)
				return status;

			widget->capabilities.input_amplifier = response;
		} else {
			// adopt capabilities from the audio function group
			widget->capabilities.input_amplifier
				= audioGroup->widget.capabilities.input_amplifier;
		}
	}

	return B_OK;
}


hda_widget*
hda_audio_group_get_widget(hda_audio_group* audioGroup, uint32 nodeID)
{
	if (audioGroup->widget_start > nodeID
		|| audioGroup->widget_start + audioGroup->widget_count < nodeID)
		return NULL;

	return &audioGroup->widgets[nodeID - audioGroup->widget_start];
}


static status_t
hda_widget_get_connections(hda_audio_group* audioGroup, hda_widget* widget)
{
	corb_t verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
		VID_GET_PARAMETER, PID_CONNECTION_LIST_LENGTH);
	uint32 response;

	if (hda_send_verbs(audioGroup->codec, &verb, &response, 1) != B_OK)
		return B_ERROR;

	uint32 listEntries = response & 0x7f;
	bool longForm = (response & 0xf0) != 0;

	if (listEntries == 0)
		return B_OK;

#if 1
	if (widget->num_inputs > 1) {
		// Get currently active connection
		verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
			VID_GET_CONNECTION_SELECT, 0);
		if (hda_send_verbs(audioGroup->codec, &verb, &response, 1) == B_OK)
			widget->active_input = response & 0xff;
	}
#endif

	uint32 valuesPerEntry = longForm ? 2 : 4;
	uint32 shift = 32 / valuesPerEntry;
	uint32 rangeMask = (1 << (shift - 1));
	int32 previousInput = -1;
	uint32 numInputs = 0;

	for (uint32 i = 0; i < listEntries; i++) {
		if ((i % valuesPerEntry) == 0) {
			// We get 2 or 4 answers per call depending on if we're
			// in short or long list mode
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_CONNECTION_LIST_ENTRY, i);
			if (hda_send_verbs(audioGroup->codec, &verb, &response, 1)
					!= B_OK) {
				ERROR("hda: Error parsing inputs for widget %ld!\n",
					widget->node_id);
				break;
			}
		}

		int32 input = (response >> (shift * (i % valuesPerEntry)))
			& ((1 << shift) - 1);

		if (input & rangeMask) {
			// found range
			input &= ~rangeMask;

			if (input < previousInput || previousInput == -1) {
				ERROR("hda: invalid range from %ld to %ld\n", previousInput,
					input);
				continue;
			}

			for (int32 rangeInput = previousInput + 1; rangeInput <= input
					&& numInputs < MAX_INPUTS; rangeInput++) {
				widget->inputs[numInputs++] = rangeInput;
			}

			previousInput = -1;
		} else if (numInputs < MAX_INPUTS) {
			// standard value
			widget->inputs[numInputs++] = input;
			previousInput = input;
		}
	}

	widget->num_inputs = numInputs;

	if (widget->num_inputs == 1)
		widget->active_input = 0;

	return B_OK;
}


static status_t
hda_widget_get_associations(hda_audio_group* audioGroup)
{
	uint32 index = 0;
	for (uint32 i = 0; i < MAX_ASSOCIATIONS; i++) {
		for (uint32 j = 0; j < audioGroup->widget_count; j++) {
			hda_widget& widget = audioGroup->widgets[j];

			if (widget.type != WT_PIN_COMPLEX)
				continue;
			if (CONF_DEFAULT_ASSOCIATION(widget.d.pin.config) != i)
				continue;
			if (audioGroup->associations[index].pin_count == 0) {
				audioGroup->associations[index].index = index;
				audioGroup->associations[index].enabled = true;
			}
			uint32 sequence = CONF_DEFAULT_SEQUENCE(widget.d.pin.config);
			if (audioGroup->associations[index].pins[sequence] != 0) {
				audioGroup->associations[index].enabled = false;
			}
			audioGroup->associations[index].pins[sequence] = widget.node_id;
			audioGroup->associations[index].pin_count++;
			if (i == 15)
				index++;
		}
		if (i != 15 && audioGroup->associations[index].pin_count != 0)
			index++;
	}
	audioGroup->association_count = index;

	return B_OK;
}


static uint32
hda_widget_prepare_pin_ctrl(hda_audio_group* audioGroup, hda_widget *widget,
	bool isOutput)
{
	uint32 ctrl = 0;
	if (isOutput)
		ctrl = PIN_ENABLE_HEAD_PHONE | PIN_ENABLE_OUTPUT;
	else
		ctrl = PIN_ENABLE_INPUT;

	if (PIN_CAP_IS_VREF_CTRL_50_CAP(widget->d.pin.capabilities)
		&& (audioGroup->codec->quirks & (isOutput ? HDA_QUIRK_OVREF50
			: HDA_QUIRK_IVREF50))) {
		ctrl |= PIN_ENABLE_VREF_50;
		TRACE("%s vref 50 enabled\n", isOutput ? "output" : "input");
	}
	if (PIN_CAP_IS_VREF_CTRL_80_CAP(widget->d.pin.capabilities)
		&& (audioGroup->codec->quirks & (isOutput ? HDA_QUIRK_OVREF80
			: HDA_QUIRK_IVREF80))) {
		ctrl |= PIN_ENABLE_VREF_80;
		TRACE("%s vref 80 enabled\n", isOutput ? "output" : "input");
	}
	if (PIN_CAP_IS_VREF_CTRL_100_CAP(widget->d.pin.capabilities)
		&& (audioGroup->codec->quirks & (isOutput ? HDA_QUIRK_OVREF100
			: HDA_QUIRK_IVREF100))) {
		ctrl |= PIN_ENABLE_VREF_100;
		TRACE("%s vref 100 enabled\n", isOutput ? "output" : "input");
	}

	return ctrl;
}


//	#pragma mark - audio group functions


static status_t
hda_codec_parse_audio_group(hda_audio_group* audioGroup)
{
	corb_t verbs[3];
	uint32 resp[3];

	hda_codec* codec = audioGroup->codec;
	uint32 codec_id = (codec->vendor_id << 16) | codec->product_id;
	hda_widget_get_stream_support(audioGroup, &audioGroup->widget);
	hda_widget_get_pm_support(audioGroup, &audioGroup->widget);
	hda_widget_get_amplifier_capabilities(audioGroup, &audioGroup->widget);

	verbs[0] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_AUDIO_GROUP_CAP);
	verbs[1] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_SUB_NODE_COUNT);

	if (hda_send_verbs(audioGroup->codec, verbs, resp, 3) != B_OK)
		return B_ERROR;

	TRACE("hda: Audio Group: Output delay: %ld samples, Input delay: %ld "
		"samples, Beep Generator: %s\n", AUDIO_GROUP_CAP_OUTPUT_DELAY(resp[0]),
		AUDIO_GROUP_CAP_INPUT_DELAY(resp[0]),
		AUDIO_GROUP_CAP_BEEPGEN(resp[0]) ? "yes" : "no");

	TRACE("hda:   #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n",
		GPIO_COUNT_NUM_GPIO(resp[1]), GPIO_COUNT_NUM_GPO(resp[1]),
		GPIO_COUNT_NUM_GPI(resp[1]), GPIO_COUNT_GPIUNSOL(resp[1]) ? "yes" : "no",
		GPIO_COUNT_GPIWAKE(resp[1]) ? "yes" : "no");
	dump_widget_stream_support(audioGroup->widget);

	audioGroup->gpio = resp[1];
	audioGroup->widget_start = SUB_NODE_COUNT_START(resp[2]);
	audioGroup->widget_count = SUB_NODE_COUNT_TOTAL(resp[2]);

	TRACE("hda:   widget start %lu, count %lu\n", audioGroup->widget_start,
		audioGroup->widget_count);

	audioGroup->widgets = (hda_widget*)calloc(audioGroup->widget_count,
		sizeof(*audioGroup->widgets));
	if (audioGroup->widgets == NULL) {
		ERROR("ERROR: Not enough memory!\n");
		return B_NO_MEMORY;
	}

	/* Iterate over all Widgets and collect info */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];
		uint32 nodeID = audioGroup->widget_start + i;
		uint32 capabilities;

		verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID, VID_GET_PARAMETER,
			PID_AUDIO_WIDGET_CAP);
		if (hda_send_verbs(audioGroup->codec, verbs, &capabilities, 1) != B_OK)
			return B_ERROR;

		widget.type = (hda_widget_type)((capabilities & AUDIO_CAP_TYPE_MASK)
			>> AUDIO_CAP_TYPE_SHIFT);

		/* Check specific node ids declared as inputs as beepers */
		switch (codec_id) {
			case 0x11d41882:
			case 0x11d41883:
			case 0x11d41884:
			case 0x11d4194a:
			case 0x11d4194b:
			case 0x11d41987:
			case 0x11d41988:
			case 0x11d4198b:
			case 0x11d4989b:
				if (nodeID == 26)
					widget.type = WT_BEEP_GENERATOR;
				break;
			case 0x10ec0260:
				if (nodeID == 23)
					widget.type = WT_BEEP_GENERATOR;
				break;
			case 0x10ec0262:
			case 0x10ec0268:
			case 0x10ec0880:
			case 0x10ec0882:
			case 0x10ec0883:
			case 0x10ec0885:
			case 0x10ec0888:
			case 0x10ec0889:
				if (nodeID == 29)
					widget.type = WT_BEEP_GENERATOR;
				break;
		}
		widget.active_input = -1;
		widget.capabilities.audio = capabilities;
		widget.node_id = nodeID;

		if ((capabilities & AUDIO_CAP_POWER_CONTROL) != 0) {
			/* We support power; switch us on! */
			verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
				VID_SET_POWER_STATE, 0);
			hda_send_verbs(audioGroup->codec, verbs, NULL, 1);

			snooze(1000);
		}
		if ((capabilities & (AUDIO_CAP_INPUT_AMPLIFIER
				| AUDIO_CAP_OUTPUT_AMPLIFIER)) != 0) {
			hda_widget_get_amplifier_capabilities(audioGroup, &widget);
		}

		TRACE("%ld: %s\n", nodeID, get_widget_type_name(widget.type));

		switch (widget.type) {
			case WT_AUDIO_OUTPUT:
			case WT_AUDIO_INPUT:
				hda_widget_get_stream_support(audioGroup, &widget);
				dump_widget_stream_support(widget);
				break;

			case WT_PIN_COMPLEX:
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
					VID_GET_PARAMETER, PID_PIN_CAP);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					widget.d.pin.capabilities = resp[0];

					TRACE("\t%s%s\n", PIN_CAP_IS_INPUT(resp[0]) ? "[Input] " : "",
						PIN_CAP_IS_OUTPUT(resp[0]) ? "[Output]" : "");
				} else {
					ERROR("%s: Error getting Pin Complex IO\n", __func__);
				}

				verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
					VID_GET_CONFIGURATION_DEFAULT, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					widget.d.pin.config = resp[0];
					const char* location =
						get_widget_location(CONF_DEFAULT_LOCATION(resp[0]));
					TRACE("\t%s, %s%s%s, %s, %s, Association:%ld\n",
						kPortConnector[CONF_DEFAULT_CONNECTIVITY(resp[0])],
						location ? location : "",
						location ? " " : "",
						kDefaultDevice[CONF_DEFAULT_DEVICE(resp[0])],
						kConnectionType[CONF_DEFAULT_CONNTYPE(resp[0])],
						kJackColor[CONF_DEFAULT_COLOR(resp[0])],
						CONF_DEFAULT_ASSOCIATION(resp[0]));
				}
				break;

			case WT_VOLUME_KNOB:
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
					VID_SET_VOLUME_KNOB_CONTROL, 0x0);
				hda_send_verbs(audioGroup->codec, verbs, NULL, 1);
				break;
			default:
				break;
		}

		hda_widget_get_pm_support(audioGroup, &widget);
		hda_widget_get_connections(audioGroup, &widget);

		dump_widget_pm_support(widget);
		dump_widget_audio_capabilities(capabilities);
		dump_widget_amplifier_capabilities(widget, true);
		dump_widget_amplifier_capabilities(widget, false);
		dump_widget_inputs(widget);
	}

	hda_widget_get_associations(audioGroup);

	// init the codecs
	switch (codec_id) {
		case 0x10ec0888: {
			hda_verb_write(codec, 0x20, VID_SET_COEFFICIENT_INDEX, 0x0);
			uint32 tmp;
			hda_verb_read(codec, 0x20, VID_GET_PROCESSING_COEFFICIENT, &tmp);
			hda_verb_write(codec, 0x20, VID_SET_COEFFICIENT_INDEX, 0x7);
			hda_verb_write(codec, 0x20, VID_SET_PROCESSING_COEFFICIENT, 
				(tmp & 0xf0) == 0x20 ? 0x830 : 0x3030);
			break;
		}
	}

	return B_OK;
}


/*! Find output path for widget */
static bool
hda_widget_find_output_path(hda_audio_group* audioGroup, hda_widget* widget,
	uint32 depth, bool &alreadyUsed)
{
	alreadyUsed = false;

	if (widget == NULL || depth > 16)
		return false;

	switch (widget->type) {
		case WT_AUDIO_OUTPUT:
			widget->flags |= WIDGET_FLAG_OUTPUT_PATH;
TRACE("      %*soutput: added output widget %ld\n", (int)depth * 2, "", widget->node_id);
			return true;

		case WT_AUDIO_MIXER:
		case WT_AUDIO_SELECTOR:
		{
			// already used
			if (widget->flags & WIDGET_FLAG_OUTPUT_PATH) {
				alreadyUsed = true;
				return false;
			}

			// search for output in this path
			bool found = false;
			for (uint32 i = 0; i < widget->num_inputs; i++) {
				hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
					widget->inputs[i]);

				if (hda_widget_find_output_path(audioGroup, inputWidget,
						depth + 1, alreadyUsed)) {
					if (widget->active_input == -1)
						widget->active_input = i;

					widget->flags |= WIDGET_FLAG_OUTPUT_PATH;
TRACE("      %*soutput: added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
					found = true;
				}
			}
if (!found) TRACE("      %*soutput: not added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
			return found;
		}

		default:
			return false;
	}
}


/*! Find input path for widget */
static bool
hda_widget_find_input_path(hda_audio_group* audioGroup, hda_widget* widget,
	uint32 depth)
{
	if (widget == NULL || depth > 16)
		return false;

	switch (widget->type) {
		case WT_PIN_COMPLEX:
			// already used
			if ((widget->flags & (WIDGET_FLAG_INPUT_PATH 
					| WIDGET_FLAG_OUTPUT_PATH)) != 0)
				return false;

			if (PIN_CAP_IS_INPUT(widget->d.pin.capabilities)) {
				switch (CONF_DEFAULT_DEVICE(widget->d.pin.config)) {
					case PIN_DEV_CD:
					case PIN_DEV_LINE_IN:
					case PIN_DEV_MIC_IN:
						widget->flags |= WIDGET_FLAG_INPUT_PATH;
TRACE("      %*sinput: added input widget %ld\n", (int)depth * 2, "", widget->node_id);
						return true;
					break;
				}
			}
			return false;
		case WT_AUDIO_INPUT:
		case WT_AUDIO_MIXER:
		case WT_AUDIO_SELECTOR:
		{
			// already used
			if ((widget->flags & (WIDGET_FLAG_INPUT_PATH 
					| WIDGET_FLAG_OUTPUT_PATH)) != 0)
				return false;

			// search for pin complex in this path
			bool found = false;
			for (uint32 i = 0; i < widget->num_inputs; i++) {
				hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
					widget->inputs[i]);

				if (hda_widget_find_input_path(audioGroup, inputWidget,
						depth + 1)) {
					if (widget->active_input == -1)
						widget->active_input = i;

					widget->flags |= WIDGET_FLAG_INPUT_PATH;
TRACE("      %*sinput: added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
					found = true;
				}
			}
if (!found) TRACE("      %*sinput: not added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
			return found;
		}

		default:
			return false;
	}
}

static bool
hda_audio_group_build_output_tree(hda_audio_group* audioGroup, bool useMixer)
{
	bool found = false;

TRACE("build output tree: %suse mixer\n", useMixer ? "" : "don't ");
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_PIN_COMPLEX
			|| !PIN_CAP_IS_OUTPUT(widget.d.pin.capabilities))
			continue;

		int device = CONF_DEFAULT_DEVICE(widget.d.pin.config);
		if (device != PIN_DEV_HEAD_PHONE_OUT
			&& device != PIN_DEV_SPEAKER
			&& device != PIN_DEV_LINE_OUT)
			continue;

TRACE("  look at pin widget %ld (%ld inputs)\n", widget.node_id, widget.num_inputs);
		for (uint32 j = 0; j < widget.num_inputs; j++) {
			hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
				widget.inputs[j]);
TRACE("    try widget %ld: %p\n", widget.inputs[j], inputWidget);
			if (inputWidget == NULL)
				continue;

			if (useMixer && inputWidget->type != WT_AUDIO_MIXER
				&& inputWidget->type != WT_AUDIO_SELECTOR)
				continue;
TRACE("    widget %ld is candidate\n", inputWidget->node_id);

			bool alreadyUsed = false;
			if (hda_widget_find_output_path(audioGroup, inputWidget, 0,
				alreadyUsed)
				|| (device == PIN_DEV_HEAD_PHONE_OUT && alreadyUsed)) {
				// find the output path to an audio output widget
				// or for headphones, an already used widget
TRACE("    add pin widget %ld\n", widget.node_id);
				if (widget.active_input == -1)
					widget.active_input = j;
				widget.flags |= WIDGET_FLAG_OUTPUT_PATH;
				found = true;
				break;
			}
		}
	}

	return found;
}


static bool
hda_audio_group_build_input_tree(hda_audio_group* audioGroup)
{
	bool found = false;

TRACE("build input tree\n");
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_AUDIO_INPUT)
			continue;

TRACE("  look at input widget %ld (%ld inputs)\n", widget.node_id, widget.num_inputs);
		for (uint32 j = 0; j < widget.num_inputs; j++) {
			hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
				widget.inputs[j]);
TRACE("    try widget %ld: %p\n", widget.inputs[j], inputWidget);
			if (inputWidget == NULL)
				continue;

TRACE("    widget %ld is candidate\n", inputWidget->node_id);

			if (hda_widget_find_input_path(audioGroup, inputWidget, 0)) {
TRACE("    add pin widget %ld\n", widget.node_id);
				if (widget.active_input == -1)
					widget.active_input = j;
				widget.flags |= WIDGET_FLAG_INPUT_PATH;
				found = true;
				break;
			}
		}
	}

	return found;
}


static status_t
hda_audio_group_build_tree(hda_audio_group* audioGroup)
{
	if (!hda_audio_group_build_output_tree(audioGroup, true)) {
		// didn't find a mixer path, try again without
TRACE("try without mixer!\n");
		if (!hda_audio_group_build_output_tree(audioGroup, false))
			return ENODEV;
	}

	if (!hda_audio_group_build_input_tree(audioGroup)) {
		ERROR("hda: build input tree failed\n");
	}

TRACE("build tree!\n");

	// select active connections

	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.active_input == -1)
			widget.active_input = 0;
		if (widget.num_inputs < 2)
			continue;

		if (widget.type != WT_AUDIO_INPUT
			&& widget.type != WT_AUDIO_SELECTOR
			&& widget.type != WT_PIN_COMPLEX)
			continue;

		corb_t verb = MAKE_VERB(audioGroup->codec->addr,
			widget.node_id, VID_SET_CONNECTION_SELECT, widget.active_input);
		if (hda_send_verbs(audioGroup->codec, &verb, NULL, 1) != B_OK)
			ERROR("hda: Setting output selector %ld failed on widget %ld!\n",
				widget.active_input, widget.node_id);
	}

	// GPIO
	uint32 gpio = 0;
	for (uint32 i = 0; i < GPIO_COUNT_NUM_GPIO(audioGroup->gpio)
		&& i < HDA_QUIRK_GPIO_COUNT; i++) {
		if (audioGroup->codec->quirks & (1 << i)) {
			gpio |= (1 << i);
		}
	}

	if (gpio != 0) {
		corb_t verb[] = {
			MAKE_VERB(audioGroup->codec->addr,
				audioGroup->widget.node_id, VID_SET_GPIO_DATA, gpio),
			MAKE_VERB(audioGroup->codec->addr,
				audioGroup->widget.node_id, VID_SET_GPIO_EN, gpio),
			MAKE_VERB(audioGroup->codec->addr,
				audioGroup->widget.node_id, VID_SET_GPIO_DIR, gpio)
		};
		TRACE("hda: Setting gpio 0x%lx\n", gpio);
		if (hda_send_verbs(audioGroup->codec, verb, NULL, 3) != B_OK)
			ERROR("hda: Setting gpio failed!\n");
	}

	dump_audiogroup_widgets(audioGroup);

	return B_OK;
}


static void
hda_codec_switch_init(hda_audio_group* audioGroup)
{
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];
		if (widget.type != WT_PIN_COMPLEX)
			continue;

		if ((widget.capabilities.audio & AUDIO_CAP_UNSOLICITED_RESPONSES) != 0
			&& (widget.d.pin.capabilities & PIN_CAP_PRES_DETECT) != 0
			&& (CONF_DEFAULT_MISC(widget.d.pin.config) & 1) == 0) {
			corb_t verb = MAKE_VERB(audioGroup->codec->addr, widget.node_id,
				VID_SET_UNSOLRESP, UNSOLRESP_ENABLE);
			hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
			TRACE("hda: Enabled unsolicited responses on widget %ld\n",
				widget.node_id);
		}
	}
}


static status_t
hda_codec_switch_handler(hda_codec* codec)
{
	while (acquire_sem(codec->unsol_response_sem) == B_OK) {
		uint32 response = codec->unsol_responses[codec->unsol_response_read++];
		codec->unsol_response_read %= MAX_CODEC_UNSOL_RESPONSES;

		bool disable = response & 1;
		hda_audio_group* audioGroup = codec->audio_groups[0];

		for (uint32 i = 0; i < audioGroup->widget_count; i++) {
			hda_widget& widget = audioGroup->widgets[i];

			if (widget.type != WT_PIN_COMPLEX
				|| !PIN_CAP_IS_OUTPUT(widget.d.pin.capabilities)
				|| CONF_DEFAULT_DEVICE(widget.d.pin.config)
					!= PIN_DEV_HEAD_PHONE_OUT)
				continue;

			corb_t verb = MAKE_VERB(audioGroup->codec->addr, widget.node_id,
				VID_GET_PINSENSE, 0);
			uint32 response;
			hda_send_verbs(audioGroup->codec, &verb, &response, 1);
			disable = response & PIN_SENSE_PRESENCE_DETECT;
			TRACE("hda: sensed pin widget %ld, %d\n", widget.node_id, disable);

			uint32 ctrl = hda_widget_prepare_pin_ctrl(audioGroup, &widget,
					true);
			verb = MAKE_VERB(audioGroup->codec->addr, widget.node_id,
				VID_SET_PIN_WIDGET_CONTROL, disable ? ctrl : 0);
			hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
			break;
		}

		for (uint32 i = 0; i < audioGroup->widget_count; i++) {
			hda_widget& widget = audioGroup->widgets[i];

			if (widget.type != WT_PIN_COMPLEX
				|| !PIN_CAP_IS_OUTPUT(widget.d.pin.capabilities))
				continue;

			int device = CONF_DEFAULT_DEVICE(widget.d.pin.config);
			if (device != PIN_DEV_AUX
				&& device != PIN_DEV_SPEAKER
				&& device != PIN_DEV_LINE_OUT)
				continue;

			uint32 ctrl = hda_widget_prepare_pin_ctrl(audioGroup, &widget,
					true);
			corb_t verb = MAKE_VERB(audioGroup->codec->addr, widget.node_id,
				VID_SET_PIN_WIDGET_CONTROL, disable ? 0 : ctrl);
			hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
		}
	}

	return B_OK;
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
	free(audioGroup->multi);
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
	audioGroup->widget.node_id = audioGroupNodeID;
	audioGroup->codec = codec;
	audioGroup->multi = (hda_multi*)calloc(1,
		sizeof(hda_multi));
	if (audioGroup->multi == NULL) {
		free(audioGroup);
		return B_NO_MEMORY;
	}
	audioGroup->multi->group = audioGroup;

	/* Parse all widgets in Audio Function Group */
	status_t status = hda_codec_parse_audio_group(audioGroup);
	if (status != B_OK)
		goto err;

	/* Setup for worst-case scenario; we cannot find any output Pin Widgets */
	status = ENODEV;

	if (hda_audio_group_build_tree(audioGroup) != B_OK)
		goto err;
	hda_codec_switch_init(audioGroup);

	audioGroup->playback_stream = hda_stream_new(audioGroup, STREAM_PLAYBACK);
	audioGroup->record_stream = hda_stream_new(audioGroup, STREAM_RECORD);
	TRACE("hda: streams playback %p, record %p\n", audioGroup->playback_stream,
		audioGroup->record_stream);

	if (audioGroup->playback_stream != NULL
		|| audioGroup->record_stream != NULL) {
		codec->audio_groups[codec->num_audio_groups++] = audioGroup;
		return B_OK;
	}

err:
	free(audioGroup->widgets);
	free(audioGroup);
	return status;
}


//	#pragma mark -


status_t
hda_audio_group_get_widgets(hda_audio_group* audioGroup, hda_stream* stream)
{
	hda_widget_type type;
	uint32 flags;

	if (stream->type == STREAM_PLAYBACK) {
		type = WT_AUDIO_OUTPUT;
		flags = WIDGET_FLAG_OUTPUT_PATH;
	} else {
		// record
		type = WT_AUDIO_INPUT;
		flags = WIDGET_FLAG_INPUT_PATH;
	}

	uint32 count = 0;

	for (uint32 i = 0; i < audioGroup->widget_count && count < MAX_IO_WIDGETS;
			i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if ((widget.flags & flags) != 0) {
			if (widget.type == WT_PIN_COMPLEX) {
				stream->pin_widget = widget.node_id;

				uint32 ctrl = hda_widget_prepare_pin_ctrl(audioGroup, &widget,
					flags == WIDGET_FLAG_OUTPUT_PATH);

TRACE("ENABLE pin widget %ld\n", widget.node_id);
				/* FIXME: Force Pin Widget to unmute; enable hp/output */
				corb_t verb = MAKE_VERB(audioGroup->codec->addr,
					widget.node_id,
					VID_SET_PIN_WIDGET_CONTROL, ctrl);
				hda_send_verbs(audioGroup->codec, &verb, NULL, 1);

				if (PIN_CAP_IS_EAPD_CAP(widget.d.pin.capabilities)) {
					uint32 result;
					verb = MAKE_VERB(audioGroup->codec->addr,
						widget.node_id, VID_GET_EAPDBTL_EN, 0);
					if (hda_send_verbs(audioGroup->codec, &verb,
						&result, 1) == B_OK) {
						result &= 0xff;
						verb = MAKE_VERB(audioGroup->codec->addr,
							widget.node_id, VID_SET_EAPDBTL_EN,
							result | EAPDBTL_ENABLE_EAPD);
						hda_send_verbs(audioGroup->codec,
							&verb, NULL, 1);
TRACE("ENABLE EAPD pin widget %ld\n", widget.node_id);
					}
				}
			}

			if (widget.capabilities.output_amplifier != 0) {
TRACE("UNMUTE/SET OUTPUT GAIN widget %ld (offset %ld)\n", widget.node_id,
	AMP_CAP_OFFSET(widget.capabilities.output_amplifier));
				corb_t verb = MAKE_VERB(audioGroup->codec->addr,
					widget.node_id,
					VID_SET_AMPLIFIER_GAIN_MUTE,
					AMP_SET_OUTPUT | AMP_SET_LEFT_CHANNEL
						| AMP_SET_RIGHT_CHANNEL
						| AMP_CAP_OFFSET(widget.capabilities.output_amplifier));
				hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
			}
			if (widget.capabilities.input_amplifier != 0) {
TRACE("UNMUTE/SET INPUT GAIN widget %ld (offset %ld)\n", widget.node_id,
	AMP_CAP_OFFSET(widget.capabilities.input_amplifier));
				for (uint32 i = 0; i < widget.num_inputs; i++) {
					corb_t verb = MAKE_VERB(audioGroup->codec->addr,
						widget.node_id,
						VID_SET_AMPLIFIER_GAIN_MUTE,
						AMP_SET_INPUT | AMP_SET_LEFT_CHANNEL
							| AMP_SET_RIGHT_CHANNEL
							| AMP_SET_INPUT_INDEX(i)
							| ((widget.active_input == (int32)i) ? 0 : AMP_MUTE)
							| AMP_CAP_OFFSET(widget.capabilities.input_amplifier));
					hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
				}
			}
		}

		if (widget.type != type || (widget.flags & flags) == 0
			|| (widget.capabilities.audio
				& (AUDIO_CAP_STEREO | AUDIO_CAP_DIGITAL)) != AUDIO_CAP_STEREO
			|| widget.d.io.formats == 0)
			continue;

		if (count == 0) {
			stream->sample_format = widget.d.io.formats;
			stream->sample_rate = widget.d.io.rates;
		} else {
			stream->sample_format &= widget.d.io.formats;
			stream->sample_rate &= widget.d.io.rates;
		}

		stream->io_widgets[count++] = widget.node_id;
	}

	if (count == 0)
		return B_ENTRY_NOT_FOUND;

	stream->num_io_widgets = count;
	return B_OK;
}


static const struct {
	uint32 subsystem_vendor_id, subsystem_id;
	uint32 codec_vendor_id, codec_id;
	uint32 quirks, nonquirks;
} hda_codec_quirks[] = {
	{ HDA_ALL, HDA_ALL, HDA_ALL, HDA_ALL, HDA_QUIRK_IVREF, 0 },
	{ HDA_ALL, HDA_ALL, HDA_ALL, HDA_ALL, HDA_QUIRK_IVREF, 0 },
	{ 0x10de, 0xcb79, CIRRUSLOGIC_VENDORID, 0x4206, HDA_QUIRK_GPIO1 | HDA_QUIRK_GPIO3, 0 },		// MacBook Pro 5.5
	{ 0x8384, 0x7680, SIGMATEL_VENDORID, 0x7680, HDA_QUIRK_GPIO0 | HDA_QUIRK_GPIO1, 0},	// Apple Intel Mac
	{ 0x106b, 0x00a1, REALTEK_VENDORID, 0x0885, HDA_QUIRK_GPIO0 | HDA_QUIRK_OVREF50, 0},	// MacBook
	{ 0x106b, 0x00a3, REALTEK_VENDORID, 0x0885, HDA_QUIRK_GPIO0, 0},	// MacBook
	{ HDA_ALL, HDA_ALL, IDT_VENDORID, 0x76b2, HDA_QUIRK_GPIO0, 0},
};


void
hda_codec_get_quirks(hda_codec* codec)
{
	codec->quirks = 0;
	uint32 subsystem_id = codec->controller->pci_info.u.h0.subsystem_id;
	uint32 subsystem_vendor_id = codec->controller->pci_info.u.h0.subsystem_vendor_id;
	uint32 codec_vendor_id = codec->vendor_id;
	uint32 codec_id = codec->product_id;
	for (uint32 i = 0; i < (sizeof(hda_codec_quirks)
		/ sizeof(hda_codec_quirks[0])); i++) {
		if (hda_codec_quirks[i].subsystem_id != 0xffffffff
			&& hda_codec_quirks[i].subsystem_id != subsystem_id)
			continue;
		if (hda_codec_quirks[i].subsystem_vendor_id != 0xffffffff
			&& hda_codec_quirks[i].subsystem_vendor_id != subsystem_vendor_id)
			continue;
		if (hda_codec_quirks[i].codec_vendor_id != 0xffffffff
			&& hda_codec_quirks[i].codec_vendor_id != codec_vendor_id)
			continue;
		if (hda_codec_quirks[i].codec_id != 0xffffffff
			&& hda_codec_quirks[i].codec_id != codec_id)
			continue;

		codec->quirks |= hda_codec_quirks[i].quirks;
		codec->quirks &= ~(hda_codec_quirks[i].nonquirks);
	}
}


void
hda_codec_delete(hda_codec* codec)
{
	if (codec == NULL)
		return;

	delete_sem(codec->response_sem);
	delete_sem(codec->unsol_response_sem);

	int32 result;
	wait_for_thread(codec->unsol_response_thread, &result);

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
	if (codec == NULL) {
		ERROR("hda: Failed to alloc a codec\n");
		return NULL;
	}

	codec->controller = controller;
	codec->addr = codecAddress;
	codec->response_sem = create_sem(0, "hda_codec_response_sem");
	if (codec->response_sem < B_OK) {
		ERROR("hda: Failed to create semaphore\n");
		goto err;
	}
	controller->codecs[codecAddress] = codec;

	codec->unsol_response_sem = create_sem(0, "hda_codec_unsol_response_sem");
	if (codec->unsol_response_sem < B_OK) {
		ERROR("hda: Failed to create semaphore\n");
		goto err;
	}
	codec->unsol_response_read = 0;
	codec->unsol_response_write = 0;

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
		PID_SUB_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, (uint32*)&response, 3) != B_OK) {
		ERROR("hda: Failed to get vendor and revision parameters\n");
		goto err;
	}

	codec->vendor_id = response.vendor;
	codec->product_id = response.device;
	codec->stepping = response.stepping;
	codec->revision = response.revision;
	codec->minor = response.minor;
	codec->major = response.major;
	hda_codec_get_quirks(codec);

	TRACE("Codec %ld Vendor: %04lx Product: %04lx, Revision: "
		"%lu.%lu.%lu.%lu\n", codecAddress, response.vendor, response.device,
		response.major, response.minor, response.revision, response.stepping);

	for (uint32 nodeID = response.start; nodeID < response.start
			+ response.count; nodeID++) {
		uint32 groupType;
		verbs[0] = MAKE_VERB(codecAddress, nodeID, VID_GET_PARAMETER,
			PID_FUNCTION_GROUP_TYPE);

		if (hda_send_verbs(codec, verbs, &groupType, 1) != B_OK) {
			ERROR("hda: Failed to get function group type\n");
			goto err;
		}

		if ((groupType & FUNCTION_GROUP_NODETYPE_MASK) == FUNCTION_GROUP_NODETYPE_AUDIO) {
			/* Found an Audio Function Group! */
			status_t status = hda_codec_new_audio_group(codec, nodeID);
			if (status != B_OK) {
				ERROR("hda: Failed to setup new audio function group (%s)!\n",
					strerror(status));
				goto err;
			}
		}
	}

	codec->unsol_response_thread = spawn_kernel_thread(
		(status_t(*)(void*))hda_codec_switch_handler,
		"hda_codec_unsol_thread", B_LOW_PRIORITY, codec);
	if (codec->unsol_response_thread < B_OK) {
		ERROR("hda: Failed to spawn thread\n");
		goto err;
	}
	resume_thread(codec->unsol_response_thread);

	return codec;
err:
	controller->codecs[codecAddress] = NULL;
	hda_codec_delete(codec);
	return NULL;
}
