/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Copyright for ali5451 support:
 *		(c) 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 */


#include "Device.h"

#include <string.h>

#include "Settings.h"
#include "Registers.h"


Device::Device(Device::Info &DeviceInfo, pci_info &PCIInfo)
		:	
		fStatus(B_ERROR),
		fPCIInfo(PCIInfo),
		fInfo(DeviceInfo),
		fIOBase(0),
		fHWSpinlock(0),
		fInterruptsNest(0),
		fBuffersReadySem(-1),
		fMixer(this),
		fPlaybackStream(this, false),
		fRecordStream(this, true)
{
	fStatus = _ReserveDeviceOnBus(true);
	if (fStatus != B_OK)
		return; // InitCheck will handle the rest

	uint32 cmdRegister = gPCI->read_pci_config(PCIInfo.bus,
							PCIInfo.device,	PCIInfo.function, PCI_command, 2);
	TRACE("cmdRegister:%#010x\n", cmdRegister);
	cmdRegister |= PCI_command_io | PCI_command_memory | PCI_command_master;
	gPCI->write_pci_config(PCIInfo.bus, PCIInfo.device,
							PCIInfo.function, PCI_command, 2, cmdRegister);

	fIOBase = PCIInfo.u.h0.base_registers[0];
	TRACE("fIOBase:%#010x\n", fIOBase);
	
	fStatus = B_OK;
}


Device::~Device()
{
	fMixer.Free();
	_ReserveDeviceOnBus(false);

	if (fBuffersReadySem > B_OK) {
		delete_sem(fBuffersReadySem);
	}
}


void
Device::_ResetCard(uint32 resetMask, uint32 releaseMask)
{
	// disable Legacy Control
	gPCI->write_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x40, 4, 0);
	uint32 cmdReg = gPCI->read_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4);
	gPCI->write_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4, cmdReg & 0xffff0000);
	snooze(100);

	// audio engine reset
	cmdReg = gPCI->read_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4);
	gPCI->write_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4, cmdReg | resetMask);
	snooze(100);

	// release reset
	cmdReg = gPCI->read_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4);
	gPCI->write_pci_config(fPCIInfo.bus, fPCIInfo.device,
						fPCIInfo.function, 0x44, 4, cmdReg & ~releaseMask);
	snooze(100);
}


status_t
Device::Setup()
{
	cpu_status cp = 0;
	uint32 channelsIndex = ChIndexMidEna | ChIndexEndEna;

	switch (HardwareId()) {
		case SiS7018:
			_ResetCard(0x000c0000, 0x00040000);

			cp = Lock();

			WritePCI32(RegSiSCodecGPIO, 0x00000000); 
			WritePCI32(RegSiSCodecStatus, SiSCodecResetOff);
			channelsIndex |= ChIndexSiSEnaB;

			Unlock(cp);
			break;

		case ALi5451:
			_ResetCard(0x000c0000, 0x00040000);

			cp = Lock();
			WritePCI32(RegALiDigiMixer, ALiDigiMixerPCMIn); 
			WritePCI32(RegALiVolumeA, 0);
			Unlock(cp);
			break;

		case TridentNX:
			_ResetCard(0x00010000, 0x00010000);

			cp = Lock();
			WritePCI32(RegNXCodecStatus, NXCodecStatusDAC1ON); 
			Unlock(cp);
			break;

		case TridentDX:
			_ResetCard(0x00040000, 0x00040000);

			cp = Lock();
			WritePCI32(RegCodecStatus, CodecStatusDACON);
			Unlock(cp);
			break;
	}

	// clear channels status
	WritePCI32(RegStopA, 0xffffffff);
	WritePCI32(RegStopB, 0xffffffff);

	// disable channels interrupt
	WritePCI32(RegEnaINTA, 0x00000000);
	WritePCI32(RegEnaINTB, 0x00000000);

	// enable loop interrupts
	WritePCI32(RegChIndex, channelsIndex);

	fRecordStream.Init();
	fPlaybackStream.Init();

	fBuffersReadySem = create_sem(0, DRIVER_NAME "_buffers_ready");

	fMixer.Init();
	
	return B_OK;
}


status_t
Device::Open(uint32 flags)
{
	TRACE("flags:%x\n", flags);

	if (atomic_add(&fInterruptsNest, 1) == 0) {
		install_io_interrupt_handler(fPCIInfo.u.h0.interrupt_line,
												InterruptHandler, this, 0);
		TRACE("Interrupt handler installed at line %d.\n",
												fPCIInfo.u.h0.interrupt_line);
	}

	status_t status = fRecordStream.Start();
	if (status != B_OK) {
		ERROR("Error of starting record stream:%#010x\n", status);
	}

	status = fPlaybackStream.Start();
	if (status != B_OK) {
		ERROR("Error of starting playback stream:%#010x\n", status);
	}

	return B_OK;
}


status_t
Device::Close()
{
	TRACE("closed!\n");

	status_t status = fPlaybackStream.Stop();
	if (status != B_OK) {
		ERROR("Error of stopping playback stream:%#010x\n", status);
	}

	status = fRecordStream.Stop();
	if (status != B_OK) {
		ERROR("Error of stopping record stream:%#010x\n", status);
	}

	if (atomic_add(&fInterruptsNest, -1) == 1) {
		remove_io_interrupt_handler(fPCIInfo.u.h0.interrupt_line,
												InterruptHandler, this);
		TRACE("Interrupt handler at line %d uninstalled.\n",
												fPCIInfo.u.h0.interrupt_line);
	}

	return B_OK;
}


status_t
Device::Free()
{
	TRACE("freed\n");
	return B_OK;
}


status_t
Device::Read(uint8 *buffer, size_t *numBytes)
{
	*numBytes = 0;
	return B_IO_ERROR;
}


status_t
Device::Write(const uint8 *buffer, size_t *numBytes)
{
	*numBytes = 0;
	return B_IO_ERROR;
}


status_t
Device::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case B_MULTI_GET_DESCRIPTION:
			return _MultiGetDescription((multi_description*)buffer);

		case B_MULTI_GET_EVENT_INFO:
			TRACE(("B_MULTI_GET_EVENT_INFO\n"));
			return B_ERROR;

		case B_MULTI_SET_EVENT_INFO:
			TRACE(("B_MULTI_SET_EVENT_INFO\n"));
			return B_ERROR;

		case B_MULTI_GET_EVENT:
			TRACE(("B_MULTI_GET_EVENT\n"));
			return B_ERROR;

		case B_MULTI_GET_ENABLED_CHANNELS:
			return _MultiGetEnabledChannels((multi_channel_enable*)buffer);

		case B_MULTI_SET_ENABLED_CHANNELS:
			return _MultiSetEnabledChannels((multi_channel_enable*)buffer);

		case B_MULTI_GET_GLOBAL_FORMAT:
			return _MultiGetGlobalFormat((multi_format_info*)buffer);

		case B_MULTI_SET_GLOBAL_FORMAT:
			return _MultiSetGlobalFormat((multi_format_info*)buffer);

		case B_MULTI_GET_CHANNEL_FORMATS:
			TRACE(("B_MULTI_GET_CHANNEL_FORMATS\n"));
			return B_ERROR;

		case B_MULTI_SET_CHANNEL_FORMATS:
			TRACE(("B_MULTI_SET_CHANNEL_FORMATS\n"));
			return B_ERROR;

		case B_MULTI_GET_MIX:
			return _MultiGetMix((multi_mix_value_info *)buffer);

		case B_MULTI_SET_MIX:
			return _MultiSetMix((multi_mix_value_info *)buffer);

		case B_MULTI_LIST_MIX_CHANNELS:
			TRACE(("B_MULTI_LIST_MIX_CHANNELS\n"));
			return B_ERROR;

		case B_MULTI_LIST_MIX_CONTROLS:
			return _MultiListMixControls((multi_mix_control_info*)buffer);

		case B_MULTI_LIST_MIX_CONNECTIONS:
			TRACE(("B_MULTI_LIST_MIX_CONNECTIONS\n"));
			return B_ERROR;

		case B_MULTI_GET_BUFFERS:
			return _MultiGetBuffers((multi_buffer_list*)buffer);

		case B_MULTI_SET_BUFFERS:
			TRACE(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR;

		case B_MULTI_SET_START_TIME:
			TRACE(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;

		case B_MULTI_BUFFER_EXCHANGE:
			return _MultiBufferExchange((multi_buffer_info*)buffer);

		case B_MULTI_BUFFER_FORCE_STOP:
			TRACE(("B_MULTI_BUFFER_FORCE_STOP\n"));
			return B_ERROR;

		default:
			ERROR("Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


status_t
Device::_MultiGetDescription(multi_description *multiDescription)
{
	multi_channel_info channel_descriptions[] = {
		{ 0, B_MULTI_OUTPUT_CHANNEL, B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,	 0 },
		{ 1, B_MULTI_OUTPUT_CHANNEL, B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
		{ 2, B_MULTI_INPUT_CHANNEL,	 B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,	 0 },
		{ 3, B_MULTI_INPUT_CHANNEL,	 B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
		{ 4, B_MULTI_OUTPUT_BUS, 	 B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,
										B_CHANNEL_MINI_JACK_STEREO },
		{ 5, B_MULTI_OUTPUT_BUS,	 B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS,
										B_CHANNEL_MINI_JACK_STEREO },
		{ 6, B_MULTI_INPUT_BUS, 	 B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,
										B_CHANNEL_MINI_JACK_STEREO },
		{ 7, B_MULTI_INPUT_BUS, 	 B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS,
										B_CHANNEL_MINI_JACK_STEREO },
	};

	multi_description Description;
	if (user_memcpy(&Description,
			multiDescription, sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	Description.interface_version = B_CURRENT_INTERFACE_VERSION;
	Description.interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strncpy(Description.friendly_name, fInfo.Name(),
									sizeof(Description.friendly_name) - 1);

	strncpy(Description.vendor_info, "Haiku.Inc.",
									sizeof(Description.vendor_info));

	Description.output_channel_count		= 2;
	Description.input_channel_count			= 2;
	Description.output_bus_channel_count	= 2;
	Description.input_bus_channel_count		= 2;
	Description.aux_bus_channel_count		= 0;

	Description.output_rates	= fMixer.OutputRates();
	Description.input_rates		= fMixer.InputRates();

	Description.output_formats	= fMixer.OutputFormats();
	Description.input_formats	= fMixer.InputFormats();

	Description.min_cvsr_rate	= 0;
	Description.max_cvsr_rate	= 0;

	Description.lock_sources = B_MULTI_LOCK_INTERNAL;
	Description.timecode_sources = 0;
	Description.interface_flags
		= B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	Description.start_latency = 3000;

	Description.control_panel[0] = '\0';

	if (user_memcpy(multiDescription,
			&Description, sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	if (Description.request_channel_count
			>= (int)(_countof(channel_descriptions))) {
		if (user_memcpy(multiDescription->channels,
					&channel_descriptions, sizeof(channel_descriptions)) != B_OK)
			return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
Device::_MultiGetEnabledChannels(multi_channel_enable *Enable)
{
	B_SET_CHANNEL(Enable->enable_bits, 0, true);
	B_SET_CHANNEL(Enable->enable_bits, 1, true);
	B_SET_CHANNEL(Enable->enable_bits, 2, true);
	B_SET_CHANNEL(Enable->enable_bits, 3, true);
	Enable->lock_source = B_MULTI_LOCK_INTERNAL;
	return B_OK;
}


status_t
Device::_MultiSetEnabledChannels(multi_channel_enable *Enable)
{
	TRACE("0:%s\n", B_TEST_CHANNEL(Enable->enable_bits, 0) ? "en" : "dis");
	TRACE("1:%s\n", B_TEST_CHANNEL(Enable->enable_bits, 1) ? "en" : "dis");
	TRACE("2:%s\n", B_TEST_CHANNEL(Enable->enable_bits, 2) ? "en" : "dis");
	TRACE("3:%s\n", B_TEST_CHANNEL(Enable->enable_bits, 3) ? "en" : "dis");
	return B_OK;
}


status_t
Device::_MultiGetGlobalFormat(multi_format_info *Format)
{
	fPlaybackStream.GetFormat(Format);
	fRecordStream.GetFormat(Format);

	return B_OK;
}


status_t
Device::_MultiSetGlobalFormat(multi_format_info *Format)
{
	status_t status = fPlaybackStream.SetFormat(Format->output, 
							fMixer.OutputFormats(), fMixer.OutputRates());
	if (status != B_OK)
		return status;

	return fRecordStream.SetFormat(Format->input,
				fMixer.InputFormats(), fMixer.InputRates());
}


status_t
Device::_MultiListMixControls(multi_mix_control_info* Info)
{
	return fMixer.ListMixControls(Info);
}


status_t
Device::_MultiGetMix(multi_mix_value_info *Info)
{
	return fMixer.GetMix(Info);
}


status_t
Device::_MultiSetMix(multi_mix_value_info *Info)
{
	return fMixer.SetMix(Info);
}


status_t
Device::_MultiGetBuffers(multi_buffer_list* List)
{
	fPlaybackStream.GetBuffers(List->flags, List->return_playback_buffers,
			List->return_playback_channels,	List->return_playback_buffer_size,
													List->playback_buffers);

	fRecordStream.GetBuffers(List->flags, List->return_record_buffers,
			List->return_record_channels, List->return_record_buffer_size,
													List->record_buffers);
	return B_OK;
}


status_t
Device::_MultiBufferExchange(multi_buffer_info* bufferInfo)
{
	multi_buffer_info BufferInfo;
	if (user_memcpy(&BufferInfo, bufferInfo, sizeof(multi_buffer_info)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	status_t status = B_NO_INIT;

	if (!fRecordStream.IsActive()) {
		status = fRecordStream.Start();
		if (status != B_OK) {
			ERROR("Error of starting record stream:%#010x\n", status);
			return status;
		}
	}

	if (!fPlaybackStream.IsActive()) {
		status = fPlaybackStream.Start();
		if (status != B_OK) {
			ERROR("Error of starting playback stream:%#010x\n", status);
			return status;
		}
	}

	status = acquire_sem_etc(fBuffersReadySem, 1,
					B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000);
	if (status == B_TIMED_OUT) {
		ERROR("Timeout during buffers exchange.\n");
	}

	cpu_status cst = Lock();

	fRecordStream.ExchangeBuffers(BufferInfo.recorded_real_time,
			BufferInfo.recorded_frames_count, BufferInfo.record_buffer_cycle);

	fPlaybackStream.ExchangeBuffers(BufferInfo.played_real_time,
			BufferInfo.played_frames_count, BufferInfo.playback_buffer_cycle);

	Unlock(cst);

	if (user_memcpy(bufferInfo, &BufferInfo, sizeof(multi_buffer_info)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


int32
Device::InterruptHandler(void *interruptParam)
{
	Device *device = reinterpret_cast<Device*>(interruptParam);
	if (device == 0) {
		ERROR("Invalid parameter in the interrupt handler.\n");
		return B_HANDLED_INTERRUPT;
	}

	bool wasHandled = false;

	acquire_spinlock(&device->fHWSpinlock);

	uint32 mask = device->ReadPCI32(RegMiscINT);
	if (mask & 0x00000020) {
		wasHandled = device->fRecordStream.InterruptHandler();
		wasHandled = device->fPlaybackStream.InterruptHandler() || wasHandled;
	}

	release_spinlock(&device->fHWSpinlock);

	return wasHandled ? B_INVOKE_SCHEDULER : B_UNHANDLED_INTERRUPT;
}


void
Device::SignalReadyBuffers()
{
	release_sem_etc(fBuffersReadySem, 1, B_DO_NOT_RESCHEDULE);
}


status_t
Device::_ReserveDeviceOnBus(bool reserve)
{
	status_t result = B_NO_INIT;
	if (reserve) {
		result = gPCI->reserve_device(fPCIInfo.bus, fPCIInfo.device,
							fPCIInfo.function, DRIVER_NAME, this);
		if (result != B_OK)
			ERROR("Unable to reserve PCI device %d:%d:%d on bus:%#010x\n",
					fPCIInfo.bus, fPCIInfo.device, fPCIInfo.function, result);
	} else {
		result = gPCI->unreserve_device(fPCIInfo.bus, fPCIInfo.device,
							fPCIInfo.function, DRIVER_NAME, this);
	}

	return result;
}


uint8
Device::ReadPCI8(int offset)
{
	return gPCI->read_io_8(fIOBase + offset);
}


uint16
Device::ReadPCI16(int offset)
{
	return gPCI->read_io_16(fIOBase + offset);
}


uint32
Device::ReadPCI32(int offset)
{
	return gPCI->read_io_32(fIOBase + offset);
}


void
Device::WritePCI8(int offset, uint8 value)
{
	gPCI->write_io_8(fIOBase + offset, value);
}


void
Device::WritePCI16(int offset, uint16 value)
{
	gPCI->write_io_16(fIOBase + offset, value);
}


void
Device::WritePCI32(int offset, uint32 value)
{
	gPCI->write_io_32(fIOBase + offset, value);
}


cpu_status
Device::Lock()
{
	cpu_status st = disable_interrupts();
	acquire_spinlock(&fHWSpinlock);
	return st;
}


void
Device::Unlock(cpu_status st)
{
	release_spinlock(&fHWSpinlock);
	restore_interrupts(st);
}


