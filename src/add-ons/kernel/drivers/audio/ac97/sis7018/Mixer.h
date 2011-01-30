/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS7018_MIXER_H_
#define _SiS7018_MIXER_H_


#include <OS.h>

#include "ac97.h"
#include "hmulti_audio.h"


class Device;
struct GainInfo;
struct MIXControlInfo;

class Mixer {

public:
					Mixer(Device *cdc);
	
		void		Init();
		void		Free();

		status_t	GetMix(multi_mix_value_info *Info);
		status_t	SetMix(multi_mix_value_info *Info);
		status_t	ListMixControls(multi_mix_control_info* Info);

		uint32		InputRates() { return fInputRates; }
		uint32		OutputRates() { return fOutputRates; }
		uint32		InputFormats() { return fInputFormats; }
		uint32		OutputFormats() { return fOutputFormats; }

		void 		SetOutputRate(uint32 rate);

private:
		void		_ReadSupportedFormats();
		bool		_WaitPortReady(uint8 reg, uint32 mask, uint32* result = NULL);
		bool		_WaitSTimerReady();
		uint16		_ReadAC97(uint8 reg);
		void		_WriteAC97(uint8 reg, uint16 date);
static	uint16		_ReadAC97(void* cookie, uint8 reg);
static	void		_WriteAC97(void* cookie, uint8 reg, uint16 data);
		bool		_CheckRegFeatures(uint8 AC97Reg, uint16& mask, uint16& result);
		bool		_CorrectMIXControlInfo(MIXControlInfo& info, GainInfo& gainInfo);
		void		_InitGainLimits(multi_mix_control& Control, GainInfo& Info);
		int32		_CreateMIXControlGroup(multi_mix_control_info* MultiInfo,
						int32& index, int32 parentIndex, MIXControlInfo& Info);

		Device*		fDevice;
		ac97_dev*	fAC97Dev;
		uint8		fReadPort;
		uint8		fWritePort;
		uint32		fMaskRW;
		uint32		fMaskRD;
		uint32		fMaskWD;

		bool		fHasVRA;
		uint32		fInputRates;
		uint32		fOutputRates;
		uint32		fInputFormats;
		uint32		fOutputFormats;
};


#endif // _SiS7018_MIXER_H_

