/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Copyright for ali5451 support:
 *		(c) 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 */


#include "Mixer.h"

#include <string.h>

#include "Device.h"
#include "Registers.h"
#include "Settings.h"


Mixer::Mixer(Device *device)
		:
		fDevice(device),
		fAC97Dev(NULL),
		fReadPort(RegCodecRead),
		fWritePort(RegCodecWrite),
		fMaskRW(1 << 15),
		fMaskRD(1 << 15),
		fMaskWD(1 << 15),
		fHasVRA(false),
		fInputRates(0),
		fOutputRates(0),
		fInputFormats(0),
		fOutputFormats(0)
{
	switch (device->HardwareId()) {
		case ALi5451:
			if (fDevice->PCIInfo().revision > 0x01) {
				fReadPort = RegCodecWrite;
				fMaskWD = 1 << 8;
			}
		case SiS7018:
			break;
		case TridentDX:
			break;
		case TridentNX:
			fReadPort	= RegNXCodecRead;
			fWritePort	= RegNXCodecWrite;
			fMaskRW		= 1 << 11;
			fMaskRD		= 1 << 10;
			fMaskWD		= 1 << 11;
			break;
	}

	TRACE("Regs:R:%#x;W:%#x;MRW:%#x;MRD:%#x;MWD:%#x\n",
			fReadPort, fWritePort, fMaskRW, fMaskRD, fMaskWD);
}


void
Mixer::Init()
{
	ac97_attach(&fAC97Dev, _ReadAC97, _WriteAC97, this,
			fDevice->PCIInfo().u.h0.subsystem_vendor_id,
			fDevice->PCIInfo().u.h0.subsystem_id);

	_ReadSupportedFormats();
}


void
Mixer::_ReadSupportedFormats()
{
	fInputRates = B_SR_48000;
	fOutputRates = 0;
	fInputFormats = B_FMT_16BIT;
	fOutputFormats = B_FMT_16BIT;

	uint16 extId = ac97_reg_cached_read(fAC97Dev, AC97_EXTENDED_ID);
	uint16 extCtrl = ac97_reg_cached_read(fAC97Dev, AC97_EXTENDED_STAT_CTRL);
	TRACE("Ext.ID:%#010x %#010x\n", extId, extCtrl);

	fHasVRA = ((extId & EXID_VRA) == EXID_VRA);
	if (!fHasVRA) {
		fOutputRates = B_SR_8000 | B_SR_11025 | B_SR_12000
						| B_SR_16000 | B_SR_22050 | B_SR_24000
						| B_SR_32000 | B_SR_44100 | B_SR_48000;
		TRACE("VRA is not supported. Assume all rates are ok:%#010x\n",
					fOutputRates);
		return;
	}

	struct _Cap {
		ac97_capability	fCap;
		uint32			fRate;
	} caps[] = {
		{ CAP_PCM_RATE_8000,  B_SR_8000 },
		{ CAP_PCM_RATE_11025, B_SR_11025 },
		{ CAP_PCM_RATE_12000, B_SR_12000 },
		{ CAP_PCM_RATE_16000, B_SR_16000 },
		{ CAP_PCM_RATE_22050, B_SR_22050 },
		{ CAP_PCM_RATE_24000, B_SR_24000 },
		{ CAP_PCM_RATE_32000, B_SR_32000 },
		{ CAP_PCM_RATE_44100, B_SR_44100 },
		{ CAP_PCM_RATE_48000, B_SR_48000 }
	};

	for (size_t i = 0; i < _countof(caps); i++) {
		if (ac97_has_capability(fAC97Dev, caps[i].fCap))
			fOutputRates |= caps[i].fRate;
	}

	if (fOutputRates == 0) {
		ERROR("Output rates are not guessed. Force to 48 kHz.\n");
		fOutputRates = B_SR_48000;
	}
	
	TRACE("Output rates are:%#010x\n", fOutputRates);
}


void
Mixer::Free()
{
	ac97_detach(fAC97Dev);
}


uint16
Mixer::_ReadAC97(void* cookie, uint8 reg)
{
	return reinterpret_cast<Mixer*>(cookie)->_ReadAC97(reg);
}


void
Mixer::_WriteAC97(void* cookie, uint8 reg, uint16 data)
{
	return reinterpret_cast<Mixer*>(cookie)->_WriteAC97(reg, data);
}


bool
Mixer::_WaitPortReady(uint8 reg, uint32 mask, uint32* result)
{
	int count = 200;
	uint32 data = 0;

	while (count--) {
		data = fDevice->ReadPCI32(reg);
		if ((data & mask) == 0) {
			if (result != NULL)
				*result = data;
			return true;
		}
		spin(1);
	}

	ERROR("AC97 register %#04x is not ready.\n", reg);
	return false;
}


bool
Mixer::_WaitSTimerReady()
{
	if (fDevice->HardwareId() != ALi5451)
		return true;

	int count = 200;
	uint32 time1 = fDevice->ReadPCI32(RegALiSTimer);

	while (count--) {
		uint32 time2 = fDevice->ReadPCI32(RegALiSTimer);
		if (time2 != time1)
			return true;
		spin(1);
	}

	ERROR("AC97 STimer is not ready.\n");
	return false;
}


uint16
Mixer::_ReadAC97(uint8 reg)
{
	uint32 result = 0;

	cpu_status cp = fDevice->Lock();

	if (_WaitPortReady(fWritePort, fMaskRD)
		&& _WaitPortReady(fReadPort, fMaskRD) && _WaitSTimerReady()) {

		fDevice->WritePCI32(fReadPort, (reg & 0x7f) | fMaskRW);

		if (_WaitSTimerReady() && _WaitPortReady(fReadPort, fMaskRD, &result))
			result = (result >> 16) & 0xffff;
	}

	fDevice->Unlock(cp);

	return result;
}


void
Mixer::_WriteAC97(uint8 reg, uint16 data)
{
	cpu_status cp = fDevice->Lock();

	if (_WaitPortReady(fWritePort, fMaskRW) && _WaitSTimerReady()) {

		fDevice->WritePCI32(fWritePort,
							(data << 16) | (reg & 0x7f) | fMaskRW | fMaskWD);
	}
	
	fDevice->Unlock(cp);
}


void
Mixer::SetOutputRate(uint32 outputRate)
{
	if (!fHasVRA)
		return;

	uint32 rate = 0;
	if (!ac97_get_rate(fAC97Dev, AC97_PCM_FRONT_DAC_RATE, &rate)) {
		ERROR("Failed to read PCM Front DAC Rate. Force to %d.\n", outputRate);
	} else
	if (rate == outputRate) {
		TRACE("AC97 PCM Front DAC rate not set to %d\n", outputRate);
		return;
	}

	if (!ac97_set_rate(fAC97Dev, AC97_PCM_FRONT_DAC_RATE, rate))
		ERROR("Failed to set AC97 PCM Front DAC rate\n");
	else
		TRACE("AC97 PCM Front DAC rate set to %d\n", outputRate);
}


// Control ids are encoded in the following way:
// GGBBRRTT --
//			GG	- in gain controls: 10th of granularity. Signed!
//				- in MUX controls: value of selector.
//			BB	- in gain controls: base level - correspond to 0 db.
//				- mute, boost, enable controls: offset of "on/off" bit
//			RR	- AC97 Register handled by this control
//			TT	- MIXControlTypes bits

const int stepShift	= 24;	// offset to GG
const int baseShift	= 16;	// offset to BB
const int regShift	= 8;	// offset to RR

enum MIXControlTypes {
	MIX_RGain	= 0x01,
	MIX_LGain	= 0x10,
	MIX_Mono	= MIX_RGain,
	MIX_Stereo	= MIX_LGain | MIX_RGain,
	MIX_Mute	= 0x04,
	MIX_Boost	= 0x08,
	MIX_MUX		= 0x20,
	MIX_Enable	= 0x40,
	MIX_Check	= MIX_Mute | MIX_Boost | MIX_Enable
};


struct GainInfo {
	uint8 fOff;		// offset of mask in register word
	uint8 fBase;	// base - default value of register
	uint8 fMask;	// mask - maximal value of register
	float fMult;	// multiplier - bits to dB recalculate
	uint8 fEnaOff;	// offset of "on/off" bit in register word
};


GainInfo MixGain	= { 0x00, 0x00, 0x3f, -1.5, 0x0f };
GainInfo InGain		= { 0x00, 0x08, 0x1f, -1.5, 0x0f };
GainInfo RecGain	= { 0x00, 0x00, 0x0f,  1.5, 0x0f };
GainInfo BeepGain	= { 0x01, 0x00, 0x1e,  3.0, 0x0f };
GainInfo ToneGain	= { 0x00, 0x07, 0x0f, -1.5, 0x10 };	// 0x10 - mean no "mute"
GainInfo D3DGain	= { 0x00, 0x00, 0x0f,  1.0, 0x10 };


struct MIXControlInfo {
		uint8		fAC97Reg;
		GainInfo*	fInfo;
		strind_id	fNameId;
		uint16		fType;
const	char*		fName;

		uint8		fExAC97Reg;
		strind_id	fExNameId;
const	char*		fExName;
		uint8		fExOff;
};


MIXControlInfo OutputControls[] = {
	{ AC97_MASTER_VOLUME,	&MixGain,	S_MASTER,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_AUX_OUT_VOLUME,	&MixGain,	S_AUX,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_MASTER_TONE,		&ToneGain,	S_OUTPUT_BASS,
		MIX_LGain, NULL },
	{ AC97_MASTER_TONE,		&ToneGain, S_OUTPUT_TREBLE,
		MIX_RGain, NULL },
	{ AC97_3D_CONTROL,		&D3DGain,	S_OUTPUT_3D_DEPTH,
		MIX_RGain | MIX_Enable, NULL,
		AC97_GENERAL_PURPOSE, S_ENABLE, NULL, 0x0d },
	{ AC97_3D_CONTROL,		&D3DGain,	S_OUTPUT_3D_CENTER,
	 	MIX_LGain, NULL },
	{ AC97_MONO_VOLUME,		&MixGain,	S_MONO_MIX,
		MIX_Mono | MIX_Mute, NULL },
	{ AC97_PC_BEEP_VOLUME,	&BeepGain,	S_BEEP,
		MIX_Mono | MIX_Mute, NULL }
};


MIXControlInfo InputControls[] = {
	{ AC97_PCM_OUT_VOLUME,	&InGain,	S_WAVE,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_MIC_VOLUME,		&InGain,	S_MIC,
		MIX_Mono | MIX_Mute | MIX_Boost, NULL,
		AC97_MIC_VOLUME, S_null, "+ 20 dB", 0x06 },
	{ AC97_LINE_IN_VOLUME,	&InGain,	S_LINE,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_CD_VOLUME,		&InGain,	S_CD,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_VIDEO_VOLUME,	&InGain,	S_VIDEO,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_AUX_IN_VOLUME,	&InGain,	S_AUX,
		MIX_Stereo | MIX_Mute, NULL },
	{ AC97_PHONE_VOLUME,	&InGain,	S_PHONE,
		MIX_Mono | MIX_Mute, NULL }
};


strind_id RecordSources[] = {
	S_MIC,
	S_CD,
	S_VIDEO,
	S_AUX,
	S_LINE,
	S_STEREO_MIX,
	S_MONO_MIX,
	S_PHONE
};


MIXControlInfo RecordControls[] = {
	{ AC97_RECORD_GAIN,		&RecGain,	S_null,
		MIX_Stereo | MIX_Mute | MIX_MUX, "Record Gain",
		AC97_RECORD_SELECT, S_null, "Source" },
	{ AC97_RECORD_GAIN_MIC,	&RecGain,	S_MIC,
		MIX_Mono | MIX_Mute, NULL }
};


void
Mixer::_InitGainLimits(multi_mix_control& Control, GainInfo& Info)
{
	float base = Info.fBase >> Info.fOff;
	float max = Info.fMask >> Info.fOff;

	float min_gain = Info.fMult * (0.0 - base);
	float max_gain = Info.fMult * (max - base);

	Control.gain.min_gain = (Info.fMult > 0) ? min_gain : max_gain;
	Control.gain.max_gain = (Info.fMult > 0) ? max_gain : min_gain;
	Control.gain.granularity = Info.fMult;

	// encode gain granularity in the MSB of the control id.
	uint8 gran = (uint8)(Info.fMult * 10.);
	Control.id |= (gran << stepShift);
	Control.id |= (Info.fBase << baseShift);

	TRACE("base:%#04x; mask:%#04x; mult:%f\n",
			Info.fBase, Info.fMask, Info.fMult);
	TRACE(" min:%f; max:%f; gran:%f -> %#010x\n",
			Control.gain.min_gain, Control.gain.max_gain,
			Control.gain.granularity, Control.id);
}


bool
Mixer::_CheckRegFeatures(uint8 AC97Reg, uint16& mask, uint16& result)
{
	uint16 backup = ac97_reg_cached_read(fAC97Dev, AC97Reg);

	ac97_reg_cached_write(fAC97Dev, AC97Reg, mask);
	result = ac97_reg_cached_read(fAC97Dev, AC97Reg);
	TRACE("Check for register %#02x: %#x -> %#x.\n", AC97Reg, mask, result);
	
	ac97_reg_cached_write(fAC97Dev, AC97Reg, backup);

	return mask != result;
}


bool
Mixer::_CorrectMIXControlInfo(MIXControlInfo& Info, GainInfo& gainInfo)
{
	uint16 newMask = gainInfo.fMask;
	uint16 testMask = 0;
	uint16 testResult = 0;
	
	switch (Info.fAC97Reg) {
		case AC97_AUX_OUT_VOLUME:
			if (ac97_has_capability(fAC97Dev, CAP_HEADPHONE_OUT)) {
				Info.fNameId = S_HEADPHONE;
				Info.fName = NULL;
			}
		case AC97_MASTER_VOLUME:
			testMask = 0x2020;
			newMask = 0x1f;
			break;
		case AC97_MASTER_TONE:
			if (!ac97_has_capability(fAC97Dev, CAP_BASS_TREBLE_CTRL))
				return false;
			testMask = 0x0f0f;
			break;
		case AC97_3D_CONTROL:
			if (!ac97_has_capability(fAC97Dev, CAP_3D_ENHANCEMENT))
				return false;
			testMask = 0x0f0f;
			break;
		case AC97_RECORD_GAIN_MIC:
			if (!ac97_has_capability(fAC97Dev, CAP_PCM_MIC))
				return false;
			testMask = 0x0f;
			break;
		case AC97_MONO_VOLUME:
			testMask = 0x20;
			newMask = 0x1f;
			break;
		case AC97_PC_BEEP_VOLUME:
			testMask = 0x1e;
			break;
		case AC97_VIDEO_VOLUME:
		case AC97_AUX_IN_VOLUME:
		case AC97_PHONE_VOLUME:
			testMask = 0x1f;
			break;
		default:
			return true;
	}
	
	if (_CheckRegFeatures(Info.fAC97Reg, testMask, testResult)) {
		if (testResult == 0)
			return false;

		gainInfo.fMask = newMask;
	}

	return true;
}


int32
Mixer::_CreateMIXControlGroup(multi_mix_control_info* MultiInfo, int32& index,
									int32 parentIndex, MIXControlInfo& Info)
{
	// check the optional registers and features,
	// correct the range if required and do not add if not supported
	GainInfo gainInfo = *Info.fInfo;
	if (!_CorrectMIXControlInfo(Info, gainInfo))
		return 0;

	int32 IdReg = Info.fAC97Reg << regShift;
	multi_mix_control* Controls = MultiInfo->controls;

	int32 groupIndex
		= Controls[index].id	= IdReg | Info.fType;
	Controls[index].flags		= B_MULTI_MIX_GROUP;
	Controls[index].parent		= parentIndex;
	Controls[index].string		= Info.fNameId;
	if (Info.fName != NULL)
		strlcpy(Controls[index].name, Info.fName,
								   sizeof(Controls[index].name) - 1);
	index++;

	if (Info.fType & MIX_Mute) {
		Controls[index].id		= IdReg | MIX_Check;
		Controls[index].id		|= Info.fInfo->fEnaOff << baseShift;
		Controls[index].flags	= B_MULTI_MIX_ENABLE;
		Controls[index].parent	= groupIndex;
		Controls[index].string	= S_MUTE;

		TRACE("Mute:%#010x\n", Controls[index].id);
		index++;
	}

	if (Info.fType & MIX_Enable) {
		int32 IdExReg = Info.fExAC97Reg << regShift;
		Controls[index].id		= IdExReg | MIX_Check;
		Controls[index].id		|= (Info.fExOff << baseShift);
		Controls[index].flags	= B_MULTI_MIX_ENABLE;
		Controls[index].parent	= groupIndex;
		Controls[index].string	= Info.fExNameId;
		if (Info.fExName != NULL)
			strlcpy(Controls[index].name, Info.fExName,
								sizeof(Controls[index].name));

		TRACE("Enable:%#010x\n", Controls[index].id);
		index++;
	}

	int32 gainIndex = 0;
	if (Info.fType & MIX_LGain) {
		Controls[index].id		= IdReg | MIX_LGain;
		Controls[index].flags	= B_MULTI_MIX_GAIN;
		Controls[index].parent	= groupIndex;
		Controls[index].string	= S_GAIN;
		_InitGainLimits(Controls[index], gainInfo);
		gainIndex = Controls[index].id;
		index++;
	}

	if (Info.fType & MIX_RGain) {
		Controls[index].id		= IdReg | MIX_RGain;
		Controls[index].flags	= B_MULTI_MIX_GAIN;
		Controls[index].parent	= groupIndex;
		Controls[index].master	= gainIndex;
		Controls[index].string	= S_GAIN;
		_InitGainLimits(Controls[index], gainInfo);
		index++;
	}

	if (Info.fType & MIX_Boost) {
		Controls[index].id		= IdReg | MIX_Check;
		Controls[index].id		|= (Info.fExOff << baseShift);
		Controls[index].flags	= B_MULTI_MIX_ENABLE;
		Controls[index].parent	= groupIndex;
		Controls[index].string	= Info.fExNameId;
		if (Info.fExName != NULL)
			strlcpy(Controls[index].name, Info.fExName,
								sizeof(Controls[index].name));

		TRACE("Boost:%#010x\n", Controls[index].id);
		index++;
	}

	if (Info.fType & MIX_MUX) {
		int32 IdMUXReg = Info.fExAC97Reg << regShift;
		int32 recordMUX
			= Controls[index].id	= IdMUXReg | MIX_MUX;
		Controls[index].flags		= B_MULTI_MIX_MUX;
		Controls[index].parent		= groupIndex;
		Controls[index].string		= S_null;
		strlcpy(Controls[index].name, Info.fExName, sizeof(Controls[index].name));

		TRACE("MUX:%#010x\n", Controls[index].id);
		index++;

		for (size_t i = 0; i < _countof(RecordSources); i++) {
			Controls[index].id		= IdMUXReg | (i << stepShift) | MIX_MUX;
			Controls[index].flags	= B_MULTI_MIX_MUX_VALUE;
			Controls[index].master	= 0;
			Controls[index].string	= RecordSources[i];
			Controls[index].parent	= recordMUX;

			TRACE("MUX Item:%#010x\n", Controls[index].id);

			index++;
		}
	}

	return groupIndex;
}


status_t
Mixer::ListMixControls(multi_mix_control_info* Info)
{
	int32 index = 0;
	multi_mix_control* Controls = Info->controls;
	int32 mixerGroup
		= Controls[index].id	= 0x8000; // 0x80 - is not a valid AC97 register
	Controls[index].flags		= B_MULTI_MIX_GROUP;
	Controls[index].parent		= 0;
	Controls[index].string		= S_OUTPUT;
	index++;

	for (size_t i = 0; i < _countof(OutputControls); i++) {
		_CreateMIXControlGroup(Info, index, mixerGroup, OutputControls[i]);	
	}

	int32 inputGroup
		= Controls[index].id	= 0x8100;
	Controls[index].flags		= B_MULTI_MIX_GROUP;
	Controls[index].parent		= 0;
	Controls[index].string		= S_INPUT;
	index++;

	for (size_t i = 0; i < _countof(InputControls); i++) {
		_CreateMIXControlGroup(Info, index, inputGroup, InputControls[i]);
	}

	int32 recordGroup
		= Controls[index].id	= 0x8200;
	Controls[index].flags		= B_MULTI_MIX_GROUP;
	Controls[index].parent		= 0;
	Controls[index].string		= S_null;
	strlcpy(Controls[index].name, "Record", sizeof(Controls[index].name));
	index++;

	for (size_t i = 0; i < _countof(RecordControls); i++) {
		_CreateMIXControlGroup(Info, index, recordGroup, RecordControls[i]);
	}

	return B_OK;
}


status_t
Mixer::GetMix(multi_mix_value_info *Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {

		int32 Id= Info->values[i].id;
		uint8 Reg = (Id >> regShift) & 0xFF;

		if ((Reg & 0x01) || Reg > 0x7e) {
			ERROR("Invalid AC97 register:%#04x.Bypass it.\n", Reg);
			continue;
		}

		uint16 RegValue = ac97_reg_cached_read(fAC97Dev, Reg);

		if ((Id & MIX_Check) == MIX_Check) {
			uint16 mask = 1 << ((Id >> baseShift) & 0xff);
			Info->values[i].enable = (RegValue & mask) == mask;
			TRACE("%#04x Mute|Enable|Boost:%d [data:%#04x]\n",
							Reg, Info->values[i].enable, RegValue);
			continue;
		}
		
		if ((Id & MIX_MUX) == MIX_MUX) {
			Info->values[i].mux = (RegValue | (RegValue >> 8)) & 0x7;
			TRACE("%#04x MUX:%d [data:%#04x]\n",
							Reg, Info->values[i].mux, RegValue);
			continue;
		}

		float mult = 0.1 * (int8)(Id >> stepShift);
		float base = mult * ((Id >> baseShift) & 0xff);

		if ((Id & MIX_RGain) == MIX_RGain) {
			uint8 gain = RegValue & 0x3f;
			Info->values[i].gain = mult * gain - base;
			TRACE("%#04x for RGain:%f [mult:%f base:%f] <- %#04x\n",
							Reg, Info->values[i].gain, mult, base, gain);
			continue;
		}

		if ((Id & MIX_LGain) == MIX_LGain) {
			uint8 gain = (RegValue >> 8) & 0x3f;
			Info->values[i].gain = mult * gain - base;
			TRACE("%#04x for LGain:%f [mult:%f base:%f] <- %#04x\n",
							Reg, Info->values[i].gain, mult, base, gain);
		}
	}

	return B_OK;
}


status_t
Mixer::SetMix(multi_mix_value_info *Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {

		int32 Id = Info->values[i].id;
		uint8 Reg = (Id >> regShift) & 0xFF;

		if ((Reg & 0x01) || Reg > 0x7e) {
			ERROR("Invalid AC97 register:%#04x.Bypass it.\n", Reg);
			continue;
		}

		uint16 RegValue = ac97_reg_cached_read(fAC97Dev, Reg);

		if ((Id & MIX_Check) == MIX_Check) {
			uint16 mask = 1 << ((Id >> baseShift) & 0xff);
			if (Info->values[i].enable)
				RegValue |= mask;
			else
				RegValue &= ~mask;
			TRACE("%#04x Mute/Enable:%d -> data:%#04x\n",
							Reg, Info->values[i].enable, RegValue);
		}
		
		if ((Id & MIX_MUX) == MIX_MUX) {
			uint8 mux = Info->values[i].mux & 0x7;
			RegValue = mux | (mux << 8);
			TRACE("%#04x MUX:%d -> data:%#04x\n",
							Reg, Info->values[i].mux, RegValue);
		}

		float mult = 0.1 * (int8)(Id >> stepShift);
		float base = mult * ((Id >> baseShift) & 0xff);

		float gain = (Info->values[i].gain + base) / mult;
		gain += (gain > 0.) ? 0.5 : -0.5;
		uint8 gainValue = (uint8)gain;

		if ((Id & MIX_RGain) == MIX_RGain) {
			RegValue &= 0xffc0;
			RegValue |= gainValue;
			
			TRACE("%#04x for RGain:%f [mult:%f base:%f] -> %#04x\n",
							Reg, Info->values[i].gain, mult, base, gainValue);
		}

		if ((Id & MIX_LGain) == MIX_LGain) {
			RegValue &= 0xc0ff;
			RegValue |= (gainValue << 8);
			TRACE("%#04x for LGain:%f [mult:%f base:%f] -> %#04x\n",
							Reg, Info->values[i].gain, mult, base, gainValue);
		}

		TRACE("%#04x Write:%#06x\n", Reg, RegValue);

		ac97_reg_cached_write(fAC97Dev, Reg, RegValue);
	}

	return B_OK;
}

