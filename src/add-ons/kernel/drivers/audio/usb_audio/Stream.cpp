/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Stream.h"
#include "Device.h"
#include "Driver.h"
#include "Settings.h"


Stream::Stream(Device *device, size_t interface, usb_interface_list *List
										/*, bool isInput, uint32 HWChannel*/)
			:
			AudioStreamingInterface(&device->AudioControl(), interface, List),
			fDevice(device),
			fStatus(B_NO_INIT),
			fStreamEndpoint(0),
			fIsRunning(false),
			fArea(-1),
			fDescriptors(0),
			fDescriptorsCount(0),
			fCurrentBuffer(0),
			fStartingFrame(0),
			fSamplesCount(0),
			fProcessedBuffers(0)/*,
			fBuffersPhysAddress(0)/ *,
			fRealTime(0),
			fFramesCount(0),
			fBufferCycle(0)*/
{

}


Stream::~Stream()
{
	delete_area(fArea);
}


status_t
Stream::Init()
{
	// lookup alternate with maximal (ch * 100 + resolution)
	uint16 maxChxRes = 0;
	for (int i = 0; i < fAlternates.Count(); i++) {
		if (fAlternates[i]->Interface() == 0) {
			TRACE("Ignore alternate %d - zero interface description.\n", i);
			continue;
		}

		if (fAlternates[i]->Format() == 0) {
			TRACE("Ignore alternate %d - zero format description.\n", i);
			continue;
		}

		if (fAlternates[i]->Format()->fFormatType != UAF_FORMAT_TYPE_I) {
			TRACE("Ignore alternate %d - format type %#02x is not supported.\n",
									i, fAlternates[i]->Format()->fFormatType);
			continue;
		}

		switch (fAlternates[i]->Interface()->fFormatTag) {
			case UAF_PCM:
			case UAF_PCM8:
			case UAF_IEEE_FLOAT:
		//	case UAF_ALAW:
		//	case UAF_MULAW:
				break;
			default:
				TRACE("Ignore alternate %d - format %#04x is not supported.\n",
									i, fAlternates[i]->Interface()->fFormatTag);
			continue;
		}

		TypeIFormatDescriptor* format
				= static_cast<TypeIFormatDescriptor*>(fAlternates[i]->Format());

		if (fAlternates[i]->Interface()->fFormatTag == UAF_PCM) {
			switch(format->fBitResolution) {
				default:
				TRACE("Ignore alternate %d - bit resolution %d "
						"is not supported.\n", i, format->fBitResolution);
					continue;
				case 8: case 16: case 18: case 20: case 24: case 32:
					break;
			}
		}

		uint16 chxRes = format->fNumChannels * 100 + format->fBitResolution;
		if (chxRes > maxChxRes) {
			maxChxRes = chxRes;
			fActiveAlternate = i;
		}
	}

	if (maxChxRes <= 0) {
		TRACE("No compatible alternate found. Stream initialization failed.\n");
		return fStatus;
	}
	const ASEndpointDescriptor* endpoint = fAlternates[
												fActiveAlternate]->Endpoint();
	fIsInput = (endpoint->fEndpointAddress & USB_ENDPOINT_ADDR_DIR_IN)
													== USB_ENDPOINT_ADDR_DIR_IN;
	TRACE("Alternate %d selected!\n", fActiveAlternate);

	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(fAlternates[
												fActiveAlternate]->Format());

	size_t bufferSize = format->fNumChannels * format->fSubframeSize;
	TRACE("bufferSize:%d\n", bufferSize);

	bufferSize *= kSamplesBufferSize;
	TRACE("bufferSize:%d\n", bufferSize);

	bufferSize *= (sizeof(usb_iso_packet_descriptor) + endpoint->fMaxPacketSize); 
	TRACE("bufferSize:%d\n", bufferSize);

	bufferSize /= endpoint->fMaxPacketSize;
	TRACE("bufferSize:%d\n", bufferSize);

	bufferSize = (bufferSize + (B_PAGE_SIZE - 1)) &~ (B_PAGE_SIZE - 1);
	TRACE("bufferSize:%d\n", bufferSize);

	fArea = create_area( (fIsInput) ? DRIVER_NAME "_record_area" :
											DRIVER_NAME "_playback_area",
								(void**)&fDescriptors, B_ANY_KERNEL_ADDRESS,
									bufferSize, B_CONTIGUOUS,
										B_READ_AREA | B_WRITE_AREA);
	if (fArea < 0) {
		TRACE_ALWAYS("Error of creating %#x - bytes size buffer area:%#010x\n",
												bufferSize, fArea);
		fStatus = fArea;
		return fStatus;
	}

	physical_entry PhysEntry;
	get_memory_map(fDescriptors, bufferSize, &PhysEntry, 1);

	TRACE_ALWAYS("Created area id: "
			"%d\naddress:%#010x[phys:%#010x]\nsize:%#010x\n",
			fArea, fDescriptors, PhysEntry.address, bufferSize);

	fDescriptorsCount = bufferSize;
	fDescriptorsCount /= (sizeof(usb_iso_packet_descriptor)
												+ endpoint->fMaxPacketSize);
	fDescriptorsCount /= kSamplesBufferCount;
	// we need same size buffers. round it!
	fDescriptorsCount *= kSamplesBufferCount;

	fSamplesCount = fDescriptorsCount * endpoint->fMaxPacketSize;
	TRACE("samplesCount:%d\n", fSamplesCount);

	fSamplesCount /= format->fNumChannels * format->fSubframeSize;
	TRACE("samplesCount:%d\n", fSamplesCount);

	for (size_t i = 0; i < fDescriptorsCount; i++) {
		fDescriptors[i].request_length = endpoint->fMaxPacketSize;
		fDescriptors[i].actual_length = 0;
		fDescriptors[i].status = B_OK;
	}

/*	uint32* b = (uint32*)(fDescriptors + fDescriptorsCount);
	for (size_t i = 0; i < fSamplesCount; i++) {
		b[i] = i * 10;
	}*/

	TRACE_ALWAYS("Descriptors count:%d\nsample size:%d\nchannels:%d:%d\n",
			fDescriptorsCount, format->fSubframeSize, format->fNumChannels,
				sizeof(usb_iso_packet_descriptor));
	return fStatus = B_OK;
}


status_t
Stream::OnSetConfiguration(usb_device device,
								const usb_configuration_info *config)
{
	if (config == NULL) {
		TRACE_ALWAYS("NULL configuration. Not set.\n");
		return B_ERROR;
	}

	usb_interface_info* interface
					= &config->interface[fInterface].alt[fActiveAlternate];
	if (interface == NULL) {
		TRACE_ALWAYS("NULL interface. Not set.\n");
		return B_ERROR;
	}

	/*status_t status =*/ gUSBModule->set_alt_interface(device, interface);
	uint8 address = fAlternates[fActiveAlternate]->Endpoint()->fEndpointAddress;

	for (size_t i = 0; i < interface->endpoint_count; i++) {
		if (address == interface->endpoint[i].descr->endpoint_address) {
			fStreamEndpoint = interface->endpoint[i].handle;
			TRACE("%s Stream Endpoint [address %#04x] handle is: %#010x.\n",
					fIsInput ? "Input" : "Output", address, fStreamEndpoint);
			return B_OK;
		}
	}

	TRACE("%s Stream Endpoint [address %#04x] was not found.\n",
			fIsInput ? "Input" : "Output", address);
	return B_ERROR;
}


status_t
Stream::Start()
{
	status_t result = B_BUSY;
	if (!fIsRunning) {
		if (!fIsInput) {
			// for (size_t i = 0; i < kSamplesBufferCount; i++)
			//	result = _QueueNextTransfer(i);
			// TODO
			//	result = _QueueNextTransfer(0);
			result = B_OK;
		} else
			result = B_OK;
		fIsRunning = result == B_OK;
	}
	return result;
}


status_t
Stream::Stop()
{
	if (fIsRunning) {
		gUSBModule->cancel_queued_transfers(fStreamEndpoint);
		fIsRunning = false;
	}

	return B_OK;
}


status_t
Stream::_QueueNextTransfer(size_t queuedBuffer)
{
	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(fAlternates[
												fActiveAlternate]->Format());

	size_t bufferSize = format->fNumChannels * format->fSubframeSize;
	bufferSize *= fSamplesCount / kSamplesBufferCount;

	uint8* buffers = (uint8*)(fDescriptors + fDescriptorsCount);

	size_t packetsCount = fDescriptorsCount / kSamplesBufferCount;

	TRACE("buffers:%#010x[%#x]\ndescrs:%#010x[%#x]\n",
			buffers + bufferSize * queuedBuffer, bufferSize,
			fDescriptors + queuedBuffer * packetsCount, packetsCount);

	return gUSBModule->queue_isochronous(fStreamEndpoint,
			buffers + bufferSize * queuedBuffer, bufferSize,
			fDescriptors + queuedBuffer * packetsCount, packetsCount,
			NULL/*&fStartingFrame*/, USB_ISO_ASAP,
			Stream::_TransferCallback, this);

	return B_OK;
}


void
Stream::_TransferCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	Stream *stream = (Stream *)cookie;

	stream->fCurrentBuffer++;
	if (stream->fCurrentBuffer >= kSamplesBufferCount) {
		stream->fCurrentBuffer = 0;
	}

	stream->_DumpDescriptors();

	stream->_DumpDescriptors();

	/*
	status_t result = stream->_QueueNextTransfer(stream->fCurrentBuffer);

	if (atomic_add(&stream->fProcessedBuffers, 1) > (int32)kSamplesBufferCount) {
		TRACE_ALWAYS("Processed buffers overflow:%d\n", stream->fProcessedBuffers);
	}
*/
	release_sem_etc(stream->fDevice->fBuffersReadySem, 1, B_DO_NOT_RESCHEDULE);

	// TRACE_ALWAYS("st:%#010x, len:%d -> %#010x\n", status, actualLength, result);
	TRACE_ALWAYS("st:%#010x, data:%#010x, len:%d\n", status, data, actualLength);

/*	if (status != B_OK) {
		TRACE_ALWAYS("Device status error:%#010x\n", status);
		status_t result = gUSBModule->clear_feature(device->fControLeNDPOint,
													USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK)
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x.\n", result);
	}
*/
}


void
Stream::_DumpDescriptors()
{
	size_t packetsCount = fDescriptorsCount / kSamplesBufferCount;
	size_t from = /*fCurrentBuffer > 0 ? packetsCount :*/ 0 ;
	size_t to   = /*fCurrentBuffer > 0 ?*/ fDescriptorsCount /*: packetsCount*/ ;
	for (size_t i = from; i < to; i++) {
		TRACE_ALWAYS("%d:req_len:%d; act_len:%d; stat:%#010x\n", i,
			fDescriptors[i].request_length,	fDescriptors[i].actual_length,
			fDescriptors[i].status);
	}
}


status_t
Stream::GetEnabledChannels(uint32& offset, multi_channel_enable *Enable)
{
	AudioChannelCluster* cluster = ChannelCluster();
	if (cluster == 0) {
		return B_ERROR;
	}

	for (size_t i = 0; i < cluster->ChannelsCount(); i++) {
		B_SET_CHANNEL(Enable->enable_bits, offset++, true);
		TRACE("Report channel %d as enabled.\n", offset);
	}

	return B_OK;
}


status_t
Stream::SetEnabledChannels(uint32& offset, multi_channel_enable *Enable)
{
	AudioChannelCluster* cluster = ChannelCluster();
	if (cluster == 0) {
		return B_ERROR;
	}

	for (size_t i = 0; i < cluster->ChannelsCount(); i++) {
		TRACE("%s channel %d.\n", (B_TEST_CHANNEL(Enable->enable_bits, offset++)
						? "Enable" : "Disable"), offset + 1);
	}

	return B_OK;
}


status_t
Stream::GetGlobalFormat(multi_format_info *Format)
{
	if (IsInput()) {
		// TODO
		Format->input.rate = B_SR_48000;
		Format->input.cvsr = 48000;
		Format->input.format = B_FMT_16BIT;
	} else {
		// TODO
		Format->output.rate = B_SR_48000;
		Format->output.cvsr = 48000;
		Format->output.format = B_FMT_16BIT;
	}

	return B_OK;
}


status_t
Stream::SetGlobalFormat(multi_format_info *Format)
{
	if (IsInput()) {
		// TODO
		TRACE("input.rate:%d\n",		Format->input.rate);
		TRACE("input.cvsr:%f\n",		Format->input.cvsr);
		TRACE("input.format:%#08x\n",	Format->input.format);
	} else {
		// TODO
		TRACE("output.rate:%d\n",		Format->output.rate);
		TRACE("output.cvsr:%f\n",		Format->output.cvsr);
		TRACE("output.format:%#08x\n",	Format->output.format);
	}

	return B_OK;
}


status_t
Stream::GetBuffers(multi_buffer_list* List)
{
// TODO: check the available buffers count!

	int32 startChannel = List->return_playback_channels;
	buffer_desc** Buffers = List->playback_buffers;

	if (fIsInput) {
		List->flags |= B_MULTI_BUFFER_RECORD;
		List->return_record_buffer_size = fSamplesCount / kSamplesBufferCount;
		List->return_record_buffers = kSamplesBufferCount;
		startChannel = List->return_record_channels;
		Buffers = List->record_buffers;

		TRACE("flags:%#10x\nreturn_record_buffer_size:%#010x\n"
			"return_record_buffers:%#010x\n", List->flags,
			List->return_record_buffer_size, List->return_record_buffers);
	} else {
		List->flags |= B_MULTI_BUFFER_PLAYBACK;
		List->return_playback_buffer_size = fSamplesCount / kSamplesBufferCount;
		List->return_playback_buffers = kSamplesBufferCount;

		TRACE("flags:%#10x\nreturn_playback_buffer_size:%#010x\n"
			"return_playback_buffers:%#010x\n", List->flags,
			List->return_playback_buffer_size, List->return_playback_buffers);
	}

	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(
						fAlternates[fActiveAlternate]->Format());
	const ASEndpointDescriptor* endpoint
					= fAlternates[fActiveAlternate]->Endpoint();

	// [buffer][channel] init buffers
	for (size_t buffer = 0; buffer < kSamplesBufferCount; buffer++) {
		TRACE("%s buffer #%d:\n", fIsInput ? "input" : "output", buffer + 1);

		for (size_t channel = startChannel;
					channel < format->fNumChannels; channel++)
		{
			// init stride to the same for all buffers
			uint32 stride = format->fSubframeSize * format->fNumChannels;
			Buffers[buffer][channel].stride = stride;

			// init to buffers area begin
			Buffers[buffer][channel].base
									= (char*)(fDescriptors + fDescriptorsCount);
			// shift for whole buffer if required
			size_t bufferSize = endpoint->fMaxPacketSize
									* (fDescriptorsCount / kSamplesBufferCount);
			Buffers[buffer][channel].base += buffer * bufferSize;
			// shift for channel if required
			Buffers[buffer][channel].base += channel * format->fSubframeSize;

			TRACE("%d:%d: base:%#010x; stride:%#010x\n", buffer, channel,
				Buffers[buffer][channel].base, Buffers[buffer][channel].stride);
		}
	}

	if (fIsInput) {
		List->return_record_channels += format->fNumChannels;
		TRACE("return_record_channels:%#010x\n", List->return_record_channels);
	} else {
		List->return_playback_channels += format->fNumChannels;
		TRACE("return_playback_channels:%#010x\n",
												List->return_playback_channels);
	}

	return B_OK;
}


/*
int32
Stream::InterruptHandler(uint32 SignaledChannelsMask)
{
	uint32 ChannelMask = 1 << fHWChannel;
	if ((SignaledChannelsMask & ChannelMask) == 0) {
		return B_UNHANDLED_INTERRUPT;
	}

	uint32 CurrentSamplePositionFlag = fDevice->ReadPCI32(TrCurSPFBReg);

	fRealTime = system_time();
	fFramesCount += fBufferSize;
	fBufferCycle = ((CurrentSamplePositionFlag & ChannelMask) == ChannelMask) ? 1 : 0;

fCSP = CurrentSamplePositionFlag;

	release_sem_etc(fDevice->fBuffersReadySem, 1, B_DO_NOT_RESCHEDULE);

	return B_HANDLED_INTERRUPT;
}*/


bool
Stream::ExchangeBuffer(multi_buffer_info* Info)
{
	if (fProcessedBuffers <= 0) {
		// looks like somebody else has processed buffers but this stream
		release_sem_etc(fDevice->fBuffersReadySem, 1, B_DO_NOT_RESCHEDULE);
		return false;
	}

	Info->played_real_time = system_time();// TODO fRealTime;
	Info->played_frames_count += fSamplesCount / kSamplesBufferCount;
	Info->playback_buffer_cycle = fCurrentBuffer;

	fCurrentBuffer++;
	fCurrentBuffer %= kSamplesBufferCount;

	atomic_add(&fProcessedBuffers, -1);

	return true;
}


/*
ASInterfaceDescriptor*
Stream::ASInterface()
{
	return fAlternates[fActiveAlternate]->Interface();
}


_ASFormatDescriptor*
Stream::ASFormat()
{
	return fAlternates[fActiveAlternate]->Format();
}*/

