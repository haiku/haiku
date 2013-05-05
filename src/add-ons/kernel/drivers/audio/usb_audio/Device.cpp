/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */

#include "Driver.h"
#include "Device.h"
#include "Settings.h"
//#include "audio.h"
#include "AudioStreamingInterface.h"
// #include "StreamFormats.h"

#include <malloc.h>

Device::Device(usb_device device)
			:
			fStatus(B_ERROR),
			fOpen(false),
			fRemoved(false),
			fInsideNotify(0),
			fDevice(device),
			fNonBlocking(false),
			fAudioControl(this),
			fControlEndpoint(0),
			fInStreamEndpoint(0),
			fOutStreamEndpoint(0),
			fNotifyReadSem(-1),
			fNotifyWriteSem(-1),
			fNotifyBuffer(NULL),
			fNotifyBufferLength(0),
			fBuffersReadySem(-1)
{
	const usb_device_descriptor* deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return;
	}

	fVendorID = deviceDescriptor->vendor_id;
	fProductID = deviceDescriptor->product_id;

	fNotifyReadSem = create_sem(0, DRIVER_NAME"_notify_read");
	if (fNotifyReadSem < B_OK) {
		TRACE_ALWAYS("Error of creating read notify semaphore:%#010x\n",
															fNotifyReadSem);
		return;
	}

	fNotifyWriteSem = create_sem(0, DRIVER_NAME"_notify_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("Error of creating write notify semaphore:%#010x\n",
															fNotifyWriteSem);
		return;
	}

	fBuffersReadySem = create_sem(0, DRIVER_NAME "_buffers_ready");
	if (fBuffersReadySem < B_OK) {
		TRACE_ALWAYS("Error of creating ready buffers semaphore:%#010x\n",
															fBuffersReadySem);
		return;
	}

	if (_SetupEndpoints() != B_OK) {
		return;
	}

	// must be set in derived class constructor
	fStatus = B_OK;
}


Device::~Device()
{
	// we have to clear because we own those objects here
/*
	for (AudioControlsIterator I = fAudioControls.Begin();
								I != fAudioControls.End(); I++) {
		delete I->Value();
	}
	fAudioControls.MakeEmpty();

	// object already freed. just purge the map
	fOutputTerminals.MakeEmpty();

	// object already freed. just purge the map
	fInputTerminals.MakeEmpty();
*/
	// free stream objects too.
	for (AudioStreamsIterator I = fStreams.Begin();
								I != fStreams.End(); I++) {
		delete *I;
	}
	fStreams.MakeEmpty();

	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);

	if (fBuffersReadySem > B_OK)
		delete_sem(fBuffersReadySem);

//	if (!fRemoved) // ???
//		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);

	if (fNotifyBuffer)
		free(fNotifyBuffer);
}


status_t
Device::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = StartDevice();
	if (result != B_OK) {
		return result;
	}

	// setup state notifications
/*	result = gUSBModule->queue_interrupt(fNotifyEndpoint, fNotifyBuffer,
								fNotifyBufferLength, _NotifyCallback, this);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of requesting notify interrupt:%#010x\n", result);
		return result;
	}
*/

	fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	return result;
}


status_t
Device::Close()
{
	if (fRemoved) {
		fOpen = false;
		return B_OK;
	}

	for (int i = 0; i < fStreams.Count(); i++) {
		fStreams[i]->Stop();
	}

	// wait until possible notification handling finished...
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);
	gUSBModule->cancel_queued_transfers(fControlEndpoint);
	gUSBModule->cancel_queued_transfers(fInStreamEndpoint);
	gUSBModule->cancel_queued_transfers(fOutStreamEndpoint);

	fOpen = false;

	return StopDevice();
}


status_t
Device::Free()
{
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

		case B_MULTI_SET_CHANNEL_FORMATS:	/* only implemented if possible */
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
			// Fill out the struct for the first time; doesn't start anything.
			return _MultiGetBuffers((multi_buffer_list*)buffer);

		case B_MULTI_SET_BUFFERS:
			// Set what buffers to use, if the driver supports soft buffers.
			TRACE(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR; /* we do not support soft buffers */

		case B_MULTI_SET_START_TIME:
			// When to actually start
			TRACE(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;

		case B_MULTI_BUFFER_EXCHANGE:
			// stop and go are derived from this being called
			return _MultiBufferExchange((multi_buffer_info*)buffer);

		case B_MULTI_BUFFER_FORCE_STOP:
			// force stop of playback, nothing in data
			return _MultiBufferForceStop();

		default:
			TRACE_ALWAYS("Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
Device::Removed()
{
	fRemoved = true;
//	fHasConnection = false;

	// the notify hook is different from the read and write hooks as it does
	// itself schedule traffic (while the other hooks only release a semaphore
	// to notify another thread which in turn safly checks for the removed
	// case) - so we must ensure that we are not inside the notify hook anymore
	// before returning, as we would otherwise violate the promise not to use
	// any of the pipes after returning from the removed hook
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);

	gUSBModule->cancel_queued_transfers(fControlEndpoint);
	gUSBModule->cancel_queued_transfers(fInStreamEndpoint);
	gUSBModule->cancel_queued_transfers(fOutStreamEndpoint);
/*
	if (fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
*/
}


status_t
Device::SetupDevice(bool deviceReplugged)
{
	return B_OK;
}


status_t
Device::CompareAndReattach(usb_device device)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("Error of getting USB device descriptor.\n");
		return B_ERROR;
	}

	if (deviceDescriptor->vendor_id != fVendorID
		&& deviceDescriptor->product_id != fProductID) {
		// this certainly isn't the same device
		return B_BAD_VALUE;
	}

	// this is the same device that was replugged - clear the removed state,
	// re- setup the endpoints and transfers and open the device if it was
	// previously opened
	fDevice = device;
	fRemoved = false;
	status_t result = _SetupEndpoints();
	if (result != B_OK) {
		fRemoved = true;
		return result;
	}

	// we need to setup hardware on device replug
	result = SetupDevice(true);
	if (result != B_OK) {
		return result;
	}

	if (fOpen) {
		fOpen = false;
		result = Open(fNonBlocking ? O_NONBLOCK : 0);
	}

	return result;
}


status_t
Device::_MultiGetDescription(multi_description *multiDescription)
{
	multi_description Description;
	if (user_memcpy(&Description, multiDescription,
				sizeof(multi_description)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	Description.interface_version = B_CURRENT_INTERFACE_VERSION;
	Description.interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strlcpy(Description.friendly_name, "USB Audio", // TODO: ????
									sizeof(Description.friendly_name));

	strlcpy(Description.vendor_info, "S.Zharski",
									sizeof(Description.vendor_info));

	Description.output_channel_count		= 0;
	Description.input_channel_count			= 0;
	Description.output_bus_channel_count	= 0;
	Description.input_bus_channel_count		= 0;
	Description.aux_bus_channel_count		= 0;

	Description.output_rates	= 0;
	Description.input_rates		= 0;

	Description.min_cvsr_rate = 0;
	Description.max_cvsr_rate = 0;

	Description.output_formats	 = 0;
	Description.input_formats	 = 0;
	Description.lock_sources	 = B_MULTI_LOCK_INTERNAL;
	Description.timecode_sources = 0;
	Description.interface_flags	 = 0;
	Description.start_latency	 = 3000;

	Description.control_panel[0] = '\0';

	AudioControlsVector USBTerminals;

	// channels (USB I/O  terminals) are already in fStreams
	// in outputs->inputs order, use them.
	for (int i = 0; i < fStreams.Count(); i++) {
		uint8 id = fStreams[i]->TerminalLink();
		_AudioControl *control = fAudioControl.Find(id);
		// if (control->SubType() == IDSOutputTerminal) {
		//	USBTerminals.PushFront(control);
		//	fStreams[i]->GetFormatsAndRates(Description);
		// } else
		// if (control->SubType() == IDSInputTerminal) {
			USBTerminals.PushBack(control);
			fStreams[i]->GetFormatsAndRates(&Description);
		// }
	}

	// int32 index = 0;
	Vector<multi_channel_info> Channels;
	/*uint32 channels =*/ fAudioControl.GetChannelsDescription(Channels, &Description, USBTerminals);
	/*uint32 bus_channels =*/ fAudioControl.GetBusChannelsDescription(Channels, &Description );

	// Description.request_channel_count = channels + bus_channels;

	TraceMultiDescription(&Description, Channels);

	if (user_memcpy(multiDescription, &Description,
				sizeof(multi_description)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// if (Description.request_channel_count >=
	//		(int)(sizeof(channel_descriptions) / sizeof(channel_descriptions[0])))
	// {
	if (user_memcpy(multiDescription->channels,
			&Channels[0], min_c(Channels.Count(),
			Description.request_channel_count)) != B_OK)
		return B_BAD_ADDRESS;
	// }

	return B_OK;
}


void
Device::TraceMultiDescription(multi_description *Description,
		Vector<multi_channel_info>& Channels)
{
	TRACE("interface_version:%d\n", Description->interface_version);
	TRACE("interface_minimum:%d\n", Description->interface_minimum);
	TRACE("friendly_name:%s\n", Description->friendly_name);
	TRACE("vendor_info:%s\n", Description->vendor_info);
	TRACE("output_channel_count:%d\n", Description->output_channel_count);
	TRACE("input_channel_count:%d\n", Description->input_channel_count);
	TRACE("output_bus_channel_count:%d\n", Description->output_bus_channel_count);
	TRACE("input_bus_channel_count:%d\n", Description->input_bus_channel_count);
	TRACE("aux_bus_channel_count:%d\n", Description->aux_bus_channel_count);
	TRACE("output_rates:%#08x\n", Description->output_rates);
	TRACE("input_rates:%#08x\n", Description->input_rates);
	TRACE("min_cvsr_rate:%f\n", Description->min_cvsr_rate);
	TRACE("max_cvsr_rate:%f\n", Description->max_cvsr_rate);
	TRACE("output_formats:%#08x\n", Description->output_formats);
	TRACE("input_formats:%#08x\n", Description->input_formats);
	TRACE("lock_sources:%d\n", Description->lock_sources);
	TRACE("timecode_sources:%d\n", Description->timecode_sources);
	TRACE("interface_flags:%#08x\n", Description->interface_flags);
	TRACE("start_latency:%d\n", Description->start_latency);
	TRACE("control_panel:%s\n", Description->control_panel);

	// multi_channel_info* Channels = Description->channels;
	// for (int i = 0; i < Description->request_channel_count; i++) {
	for (int i = 0; i < Channels.Count(); i++) {
		TRACE(" channel_id:%d\n", Channels[i].channel_id);
		TRACE("  kind:%#02x\n", Channels[i].kind);
		TRACE("  designations:%#08x\n", Channels[i].designations);
		TRACE("  connectors:%#08x\n", Channels[i].connectors);
	}

	TRACE("request_channel_count:%d\n\n", Description->request_channel_count);
}


status_t
Device::_MultiGetEnabledChannels(multi_channel_enable *Enable)
{
	status_t status = B_OK;

	Enable->lock_source = B_MULTI_LOCK_INTERNAL;

	uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++) {
		status = fStreams[i]->GetEnabledChannels(offset, Enable);
	}

	return status;
}


status_t
Device::_MultiSetEnabledChannels(multi_channel_enable *Enable)
{
	status_t status = B_OK;
	uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++) {
		status = fStreams[i]->SetEnabledChannels(offset, Enable);
	}

	return status;
}


status_t
Device::_MultiGetGlobalFormat(multi_format_info *Format)
{
	status_t status = B_OK;

	Format->output_latency = 0;
	Format->input_latency = 0;
	Format->timecode_kind = 0;

	// uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++) {
		status = fStreams[i]->GetGlobalFormat(Format);
	}

	return status;
}


status_t
Device::_MultiSetGlobalFormat(multi_format_info *Format)
{
	status_t status = B_OK;

	TRACE("output_latency:%lld\n", Format->output_latency);
	TRACE("input_latency:%lld\n",  Format->input_latency);
	TRACE("timecode_kind:%#08x\n", Format->timecode_kind);

	// uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++) {
		status = fStreams[i]->SetGlobalFormat(Format);
	}

	return status;
}


status_t
Device::_MultiGetBuffers(multi_buffer_list* List)
{
	status_t status = B_OK;

	TRACE("info_size:%d\n"
	"request_playback_buffers:%d\n"
	"request_playback_channels:%d\n"
	"request_playback_buffer_size:%d\n"
	"request_record_buffers:%d\n"
	"request_record_channels:%d\n"
	"request_record_buffer_size:%d\n",
		List->info_size,
		List->request_playback_buffers,
		List->request_playback_channels,
		List->request_playback_buffer_size,
		List->request_record_buffers,
		List->request_record_channels,
		List->request_record_buffer_size);

	for (int i = 0; i < fStreams.Count() && status == B_OK; i++) {
		status = fStreams[i]->GetBuffers(List);
	}

	return B_OK;
}


status_t
Device::_MultiBufferExchange(multi_buffer_info* Info)
{
	for (int i = 0; i < fStreams.Count(); i++) {
		if (!fStreams[i]->IsRunning()) {
			fStreams[i]->Start();
		}
	}

	TRACE_ALWAYS("Exchange!\n");
	snooze(1000000);
	return B_OK;

	status_t status = B_ERROR;
	bool anyBufferProcessed = false;
	for (int i = 0; i < fStreams.Count() && !anyBufferProcessed; i++) {
		status = acquire_sem_etc(fBuffersReadySem, 1,
							B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000);
		if (status == B_TIMED_OUT) {
			TRACE_ALWAYS("Timeout during buffers exchange.\n");
			break;
		}

		anyBufferProcessed = fStreams[i]->ExchangeBuffer(Info);
		status = anyBufferProcessed ? B_OK : B_ERROR;
	}

	return status;
}


status_t
Device::_MultiBufferForceStop()
{
	for (int i = 0; i < fStreams.Count(); i++) {
		fStreams[i]->Stop();
	}
	return B_OK;
}


status_t
Device::_MultiGetMix(multi_mix_value_info *Info)
{
	return fAudioControl.GetMix(Info);
}


status_t
Device::_MultiSetMix(multi_mix_value_info *Info)
{
	return fAudioControl.SetMix(Info);
}


status_t
Device::_MultiListMixControls(multi_mix_control_info* Info)
{
	status_t status = fAudioControl.ListMixControls(Info);
	TraceListMixControls(Info);
	return status;
}


void
Device::TraceListMixControls(multi_mix_control_info *Info)
{
	TRACE("control_count:%d\n.", Info->control_count);

	int32 i = 0;
	while (Info->controls[i].id > 0) {
		multi_mix_control &c = Info->controls[i];
		TRACE("id:%#08x\n",		c.id);
		TRACE("flags:%#08x\n",	c.flags);
		TRACE("master:%#08x\n", c.master);
		TRACE("parent:%#08x\n", c.parent);
		TRACE("string:%d\n",	c.string);
		TRACE("name:%s\n",		c.name);
		i++;
	}
}


status_t
Device::_SetupEndpoints()
{
	const usb_configuration_info *config
		= gUSBModule->get_nth_configuration(fDevice, 0);

	if (config == NULL) {
		TRACE_ALWAYS("Error of getting USB device configuration.\n");
		return B_ERROR;
	}

	if (config->interface_count <= 0) {
		TRACE_ALWAYS("Error:no interfaces found in USB device configuration\n");
		return B_ERROR;
	}

	for (size_t i = 0; i < config->interface_count; i++) {
		usb_interface_info *Interface = config->interface[i].active;
		if (Interface->descr->interface_class != UAS_AUDIO) {
			continue;
		}

		switch (Interface->descr->interface_subclass) {
			case UAS_AUDIOCONTROL:
				fAudioControl.Init(i, Interface);
				break;
			case UAS_AUDIOSTREAMING:
				{
					Stream *stream = new Stream(this, i, &config->interface[i]);
					if (B_OK == stream->Init()) {
						// put the stream in the correct order:
						// first output that input ones.
						if (stream->IsInput()) {
							fStreams.PushBack(stream);
						} else {
							fStreams.PushFront(stream);
						}
					} else {
						delete stream;
					}
				}
				break;
			default:
				TRACE_ALWAYS("Ignore interface of unsupported subclass %#x.\n",
									Interface->descr->interface_subclass);
				break;
		}
	}

	if (fAudioControl.InitCheck() == B_OK && fStreams.Count() > 0) {
		TRACE("Found device %#06x:%#06x\n", fVendorID, fProductID);
		gUSBModule->set_configuration(fDevice, config);

		for (int i = 0; i < fStreams.Count(); i++) {
			fStreams[i]->OnSetConfiguration(fDevice, config);
		}

		return B_OK;
	}

	return B_NO_INIT;
}


status_t
Device::StopDevice()
{
	status_t result = B_OK; // WriteRXControlRegister(0);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n", 0, result);
	}

	TRACE_RET(result);
	return result;
}


void
Device::_ReadCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("ReadCB: %d bytes; status:%#010x\n", actualLength, status);
	Device *device = (Device *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
Device::_WriteCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	TRACE_FLOW("WriteCB: %d bytes; status:%#010x\n", actualLength, status);
	Device *device = (Device *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
Device::_NotifyCallback(void *cookie, int32 status, void *data,
	uint32 actualLength)
{
	Device *device = (Device *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->fRemoved) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

/*	if (status != B_OK) {
		TRACE_ALWAYS("Device status error:%#010x\n", status);
		status_t result = gUSBModule->clear_feature(device->fControLeNDPOint,
													USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK)
			TRACE_ALWAYS("Error during clearing of HALT state:%#010x.\n", result);
	}
*/
	// parse data in overriden class
//	device->OnNotify(actualLength);

	// schedule next notification buffer
//	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
//		device->fNotifyBufferLength, _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}

