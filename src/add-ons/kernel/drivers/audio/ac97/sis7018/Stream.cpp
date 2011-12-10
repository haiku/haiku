/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Copyright for ali5451 support:
 *		(c) 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 */


#include "Stream.h"

#include <memory.h>

#include "Device.h"
#include "Registers.h"
#include "Settings.h"


Stream::Stream(Device *device, bool isInput)
		:	
		fDevice(device),
		fStatus(B_NO_INIT),
		fIsInput(isInput),
		fIsActive(false),
		fHWChannel(isInput ? 0 : 1),
		fBuffersArea(-1),
		fBuffersAreaSize(0),
		fBufferSamplesCount(0),
		fBuffersAddress(0),
		fBuffersPhysAddress(0),
		fRealTime(0),
		fFramesCount(0),
		fBufferCycle(fIsInput ? 1 :0)
{
	fFormat.format = B_FMT_16BIT;
	fFormat.rate = B_SR_48000;
	fFormat.cvsr = _DecodeRate(fFormat.rate);
	memset(fFormat._reserved_, 0, sizeof(fFormat._reserved_));
}


Stream::~Stream()
{
	Free();
}


uint32
Stream::_HWId()
{
	return fDevice->HardwareId();
}


status_t
Stream::Init()
{
	if (fStatus == B_OK)
		Free();

	fHWChannel = fIsInput ? 0 : 1;

	if (_HWId() == SiS7018)
			fHWChannel += 0x20; // bank B optimized for PCM
	else if (_HWId() == ALi5451 && fIsInput)
			fHWChannel = 31;
	
	// assume maximal possible buffers size
	fBuffersAreaSize = 1024; // samples
	fBuffersAreaSize *= 2 * 2 * 2; // stereo + 16-bit samples + 2 buffers
	fBuffersAreaSize = (fBuffersAreaSize + (B_PAGE_SIZE - 1)) &~ (B_PAGE_SIZE - 1);
	fBuffersArea = create_area(
			(fIsInput) ? DRIVER_NAME "_record_area" : DRIVER_NAME "_playback_area",
				&fBuffersAddress, B_ANY_KERNEL_ADDRESS, fBuffersAreaSize,
				B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (fBuffersArea < 0) {
		ERROR("Error of creating %#lx-bytes size buffer area:%#010x\n",
												fBuffersAreaSize, fBuffersArea);
		fStatus = fBuffersArea;
		return fStatus;
	}

	physical_entry PhysEntry;
	get_memory_map(fBuffersAddress, fBuffersAreaSize, &PhysEntry, 1);

	fBuffersPhysAddress = PhysEntry.address;

	TRACE("Created area id %d with size: %#x at address %#x[phys:%#lx]\n",
		fBuffersArea, fBuffersAreaSize, fBuffersAddress, fBuffersPhysAddress);

	// back to samples - half of buffer for 16-bit stereo data
	fBufferSamplesCount = fBuffersAreaSize / (2 * 2 * 2);

	fStatus = B_OK;
	return fStatus;
}


void
Stream::Free()
{
	delete_area(fBuffersArea);
	fStatus = B_NO_INIT;
}


uint32
Stream::_DecodeRate(uint32 rate)
{
	switch(rate) {
		case B_SR_8000: return 8000;
		case B_SR_11025: return 11025;
		case B_SR_12000: return 12000;
		case B_SR_16000: return 16000;
		case B_SR_22050: return 22050;
		case B_SR_24000: return 24000;
		case B_SR_32000: return 32000;
		case B_SR_44100: return 44100;
		case B_SR_48000: return 48000;
	}

	ERROR("Rate:%x is not supported. Fall to default 48000\n", rate);
	return 48000;
}


void	
Stream::GetFormat(multi_format_info *Format)	
{	
	if (fIsInput) {	
		Format->input_latency = 0;	
		Format->input = fFormat;	
	} else {	
		Format->output_latency = 0;	
		Format->output = fFormat;	
	}	
}	


status_t
Stream::SetFormat(_multi_format& format, uint32 formats, uint32 rates)
{
	if (fFormat.rate == format.rate && fFormat.format == format.format)
		return B_OK;

	if ((format.rate & rates) == 0 || (format.format & formats) == 0) {
		ERROR("Unsupported data format:%x or rate:%x. Ignore.\n",
					format.format, format.rate);
		return B_ERROR;
	}
	
	fFormat = format;
	fFormat.cvsr = _DecodeRate(fFormat.rate);
	
	fBufferSamplesCount = fBuffersAreaSize / (2 * 2);
	switch (fFormat.format) {
		default:
			ERROR("Unsupported data format:%x. 16 bit assumed.\n", fFormat.format);
		case B_FMT_16BIT:
			fBufferSamplesCount /= 2;
			break;
		case B_FMT_8BIT_S:
		case B_FMT_8BIT_U:
			break;
	}

	TRACE("Format:%#x;Rate:%#x;cvsr:%.2f\n",
			fFormat.format, fFormat.rate, fFormat.cvsr);
	
	// stop the stream - it will be rewaked during next exchnage buffers call
	Stop();

	return B_OK;
}


void
Stream::GetBuffers(uint32& Flags, int32& BuffersCount, int32& ChannelsCount,
						uint32& BufferSize, buffer_desc** Buffers)
{
	Flags |= fIsInput ? B_MULTI_BUFFER_RECORD : B_MULTI_BUFFER_PLAYBACK;
	BuffersCount = 2;
	ChannelsCount = 2;
	BufferSize = fBufferSamplesCount;

	uint32 stride = 4;
	if (fFormat.format == B_FMT_8BIT_S || fFormat.format == B_FMT_8BIT_U) {
		stride = 2;
	}
		// [b][c] init buffers
	Buffers[0][0].base
		= Buffers[1][0].base
		= Buffers[0][1].base
		= Buffers[1][1].base = (char*)fBuffersAddress;

	Buffers[0][0].stride
		= Buffers[1][0].stride
		= Buffers[0][1].stride
		= Buffers[1][1].stride = stride;

	// shift pair of second part of buffers
	Buffers[1][0].base += BufferSize * stride;
	Buffers[1][1].base += BufferSize * stride;

	// shift right channel buffers
	Buffers[0][1].base += stride / 2;
	Buffers[1][1].base += stride / 2;

	TRACE("%s buffers:\n", fIsInput ? "input" : "output");
	TRACE("1: %#010x %#010x\n", Buffers[0][0].base, Buffers[0][1].base);
	TRACE("2: %#010x %#010x\n", Buffers[1][0].base, Buffers[1][1].base);
}


status_t
Stream::Start()
{
	if (!fIsInput)
		fDevice->Mixer().SetOutputRate(fFormat.cvsr);

	uint32 CSO = 0;
	uint32 LBA = uint32(fBuffersPhysAddress) & 0x3fffffff;
	uint32 ESO = ((fBufferSamplesCount * 2) - 1) & 0xffff;
	uint32 Delta = fIsInput ? ((48000 << 12) / uint32(fFormat.cvsr)) & 0xffff
							: ((uint32(fFormat.cvsr) << 12) / 48000) & 0xffff;
	uint32 DeltaESO = (ESO << 16) | Delta;
	uint32 FMControlEtc = fIsInput ? 0 : (0x03 << 14);
	uint32 ControlEtc =  1 << 14 | 1 << 12; // stereo data + loop enabled
	
	switch (fFormat.format) {
		case B_FMT_16BIT:
			ControlEtc |= (1 << 15); // 16 bit
		case B_FMT_8BIT_S:
			ControlEtc |= (1 << 13); // signed
			break;
	}

	switch (_HWId()) {
		case TridentDX:
			FMControlEtc |= (0x7f << 7) < 0x7f;
			break;
		case TridentNX:
			CSO = Delta << 24;
			DeltaESO = ((Delta << 16) & 0xff000000) | (ESO & 0x00ffffff);
			FMControlEtc |= (0x7f << 7) < 0x7f;
			break;
		case SiS7018:
			FMControlEtc = fIsInput ? (0x8a80 << 16) : FMControlEtc;
			break;
	}

	cpu_status cst = fDevice->Lock();

	// select used channel
	uint32 ChIntReg = fDevice->ReadPCI32(RegChIndex) & ~0x3f;
	ChIntReg |= (fHWChannel & 0x3f);
	fDevice->WritePCI32(RegChIndex, ChIntReg);

	fDevice->WritePCI32(RegCSOAlphaFMS, CSO);
	fDevice->WritePCI32(RegLBA_PPTR, LBA);
	fDevice->WritePCI32(RegDeltaESO, DeltaESO);
	fDevice->WritePCI32(RegRVolCVolFMC, FMControlEtc);
	fDevice->WritePCI32(RegGVSelVolCtrl, ControlEtc);
	fDevice->WritePCI32(RegEBuf1, 0);
	fDevice->WritePCI32(RegEBuf2, 0);

	if (fIsInput) {
		uint32 reg = 0;
		switch (_HWId()) {
			case ALi5451:
				reg = fDevice->ReadPCI32(RegALiDigiMixer);
				fDevice->WritePCI32(RegALiDigiMixer, reg | (1 << _HWVoice()));
				break;
			case TridentDX:
				reg = fDevice->ReadPCI8(RegCodecStatus);
				fDevice->WritePCI8(RegCodecStatus,
						reg | CodecStatusADCON | CodecStatusSBCtrl);
				// enable and set record channel
				fDevice->WritePCI8(RegRecChannel, 0x80 | _HWVoice());
				break;
			case TridentNX:
				reg = fDevice->ReadPCI16(RegMiscINT);
				fDevice->WritePCI8(RegMiscINT, reg | 0x1000);
				// enable and set record channel
				fDevice->WritePCI8(RegRecChannel, 0x80 | _HWVoice());
				break;
		}
	}

	// enable INT for current channel
	uint32 ChIntMask = fDevice->ReadPCI32(_UseBankB() ? RegEnaINTB : RegEnaINTA);
	ChIntMask |= 1 << _HWVoice();
	fDevice->WritePCI32(_UseBankB() ? RegAddrINTB : RegAddrINTA, 1 << _HWVoice());
	fDevice->WritePCI32(_UseBankB() ? RegEnaINTB : RegEnaINTA, ChIntMask);

	// start current channel
	fDevice->WritePCI32(_UseBankB() ? RegStartB : RegStartA, 1 << _HWVoice());
	fIsActive = true;

	fDevice->Unlock(cst);

	TRACE("%s:CSO:%#x;LBA:%#x;ESO:%#x;Delta:%#x;FM:%#x:Ctrl:%#x;CIR:%#x\n",
		fIsInput ? "Rec" : "Play", CSO, LBA, ESO, Delta, FMControlEtc,
			ControlEtc, ChIntReg);
	
	return B_OK;
}


status_t
Stream::Stop()
{
	if (!fIsActive)
		return B_OK;

	cpu_status cst = fDevice->Lock();

	// stop current channel
	fDevice->WritePCI32(_UseBankB() ? RegStopB : RegStopA, 1 << _HWVoice());
	fIsActive = false;
	
	if (_HWId() == ALi5451 && fIsInput) {
		uint32 reg = fDevice->ReadPCI32(RegALiDigiMixer);
		fDevice->WritePCI32(RegALiDigiMixer, reg & ~(1 << _HWVoice()));
	}

	fDevice->Unlock(cst);

	TRACE("%s:OK\n", fIsInput ? "Rec" : "Play");
	
	fBufferCycle = fIsInput ? 1 : 0;
	
	return B_OK;
}


bool
Stream::InterruptHandler()
{
	uint32 SignaledMask = fDevice->ReadPCI32(
							_UseBankB() ? RegAddrINTB : RegAddrINTA);
	uint32 ChannelMask = 1 << _HWVoice();
	if ((SignaledMask & ChannelMask) == 0) {
		return false;
	}

	// first clear signalled channel bit
	fDevice->WritePCI32(_UseBankB() ? RegAddrINTB : RegAddrINTA, ChannelMask);

	fRealTime = system_time();
	fFramesCount += fBufferSamplesCount;
	fBufferCycle = (fBufferCycle + 1) % 2;

	fDevice->SignalReadyBuffers();

	return true;
}


void
Stream::ExchangeBuffers(bigtime_t& RealTime,
								bigtime_t& FramesCount, int32& BufferCycle)
{
	RealTime = fRealTime;
	FramesCount = fFramesCount;
	BufferCycle = fBufferCycle;
}

