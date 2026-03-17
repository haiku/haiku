/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ATKeymap.h"

#include "HyperVKeyboard.h"


HyperVKeyboard::HyperVKeyboard(device_node* node)
	:
	fNode(node),
	fStatus(B_NO_INIT),
	fOpenCount(0),
	fPacket(NULL),
	fKeyBuffer(NULL),
	fKeyBufferSem(-1),
	fHyperV(NULL),
	fHyperVCookie(NULL)
{
	CALLED();

	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info**)&fHyperV, (void**)&fHyperVCookie);
	gDeviceManager->put_node(parent);

	fProtocolRespEvent.Init(this, "hyperv_keyboard protocol");
	mutex_init(&fKeyBufferLock, "hyperv_keyboard keybuffer");

	fKeyBufferSem = create_sem(0, "hyperv_keyboard sem");
	if (fKeyBufferSem < 0) {
		fStatus = fKeyBufferSem;
		return;
	}

	fPacket = static_cast<uint8*>(malloc(HV_KEYBOARD_RX_PKT_BUFFER_SIZE));
	if (fPacket == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fKeyBuffer = create_ring_buffer(HYPERV_KEYBOARD_KEY_BUFFER_SIZE * sizeof(raw_key_info));
	if (fKeyBuffer == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = fHyperV->open(fHyperVCookie, HV_KEYBOARD_TX_RING_SIZE, HV_KEYBOARD_RX_RING_SIZE,
		_CallbackHandler, this);
	if (fStatus != B_OK) {
		ERROR("Failed to open channel (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _Connect();
	if (fStatus != B_OK) {
		ERROR("Failed to connect to keyboard (%s)\n", strerror(fStatus));
		return;
	}
}


HyperVKeyboard::~HyperVKeyboard()
{
	CALLED();

	fHyperV->close(fHyperVCookie);

	free(fPacket);
	delete_sem(fKeyBufferSem);
	mutex_destroy(&fKeyBufferLock);
	delete_ring_buffer(fKeyBuffer);
}


status_t
HyperVKeyboard::Register()
{
	int32 id = gDeviceManager->create_id(HYPERV_KEYBOARD_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "input/keyboard/hyperv/%" B_PRId32, id);

	status_t status = gDeviceManager->publish_device(fNode, name,
		HYPERV_KEYBOARD_DEVICE_MODULE_NAME);
	if (status != B_OK)
		gDeviceManager->free_id(HYPERV_KEYBOARD_DEVICE_ID_GENERATOR, id);

	return status;
}


status_t
HyperVKeyboard::Open()
{
	atomic_add(&fOpenCount, 1);
	TRACE("Open count %d (open)\n", atomic_get(&fOpenCount));
	return B_OK;
}


status_t
HyperVKeyboard::Close()
{
	atomic_add(&fOpenCount, -1);
	TRACE("Open count %d (close)\n", atomic_get(&fOpenCount));
	return B_OK;
}


status_t
HyperVKeyboard::Control(uint32 op, void* buffer, size_t length)
{
	switch (op) {
		case KB_READ:
		{
			raw_key_info keyInfo;
			if (length < sizeof(keyInfo))
				return B_BAD_VALUE;

			status_t status = _ReadKeystroke(&keyInfo);
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &keyInfo, sizeof(keyInfo));
		}

		case KB_SET_LEDS:
		{
			led_info ledInfo;
			if (length < sizeof(ledInfo))
				return B_BAD_VALUE;

			if (user_memcpy(&ledInfo, buffer, sizeof(ledInfo)) != B_OK)
				return B_BAD_ADDRESS;

			return _SetIndicators(ledInfo);
		}
	}

	TRACE("Invalid ioctl %u\n", op);
	return B_DEV_INVALID_IOCTL;
}


/*static*/ void
HyperVKeyboard::_CallbackHandler(void* data)
{
	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(data);
	keyboard->_Callback();
}


void
HyperVKeyboard::_Callback()
{
	while (true) {
		uint32 length = HV_KEYBOARD_RX_PKT_BUFFER_SIZE;
		uint32 headerLength;
		uint32 messageLength;

		status_t status = fHyperV->read_packet(fHyperVCookie, fPacket, &length, &headerLength,
			&messageLength);
		if (status == B_DEV_NOT_READY) {
			break;
		} else if (status != B_OK) {
			ERROR("Failed to read packet (%s)\n", strerror(status));
			break;
		}

		hv_keyboard_msg* message = reinterpret_cast<hv_keyboard_msg*>(fPacket + headerLength);
		if (messageLength < sizeof(message->header)) {
			ERROR("Invalid keyboard message received\n");
			continue;
		}

		switch (message->header.type) {
			case HV_KEYBOARD_MSGTYPE_PROTOCOL_RESPONSE:
				if (messageLength < sizeof(message->protocol_response)) {
					ERROR("Invalid protocol response message received\n");
					break;
				}

				fProtocolRespEvent.NotifyAll(((message->protocol_response.status
					& HV_KEYBOARD_PROTOCOL_STATUS_OK) != 0) ? B_OK : B_NOT_SUPPORTED);
				break;

			case HV_KEYBOARD_MSGTYPE_EVENT:
				if (messageLength < sizeof(message->event)) {
					ERROR("Invalid event message received\n");
					break;
				}

				_HandleEvent(&message->event);
				break;

			default:
				TRACE("Unknown message type %u\n", message->header.type);
				break;
		}
	}
}


void
HyperVKeyboard::_HandleEvent(hv_keyboard_msg_event* message)
{
	TRACE("New %s%s%s%s code 0x%X\n", message->is_e0 ? "e0 " : "", message->is_e1 ? "e1 " : "",
		message->is_unicode ? "unicode " : "", message->is_break ? "break" : "make",
		message->keycode);

	// Ignore if no clients
	if (atomic_get(&fOpenCount) == 0)
		return;

	if (message->keycode == 0)
		return;

	raw_key_info keyInfo;
	keyInfo.timestamp = system_time();
	keyInfo.is_keydown = !message->is_break;

	if (message->is_unicode) {
		// TODO: Unicode keycode (Hyper-V paste clipboard)
		return;
	} else {
		// PS/2 keycode (normal keystrokes)
		uint8 keycode = static_cast<uint8>(message->keycode);
		if (message->is_e0)
			keycode |= 0x80;
		keyInfo.keycode = kATKeycodeMap[keycode - 1];
	}

	_WriteKeystroke(&keyInfo);
}


status_t
HyperVKeyboard::_Connect()
{
	hv_keyboard_msg_protocol_request message;
	message.header.type = HV_KEYBOARD_MSGTYPE_PROTOCOL_REQUEST;
	message.version = HV_KEYBOARD_VERSION_V1_0;

	ConditionVariableEntry entry;
	fProtocolRespEvent.Add(&entry);

	status_t status = fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, &message,
		sizeof(message), true, (uint64)&message);
	if (status != B_OK)
		return status;

	return entry.Wait(B_RELATIVE_TIMEOUT, HV_KEYBOARD_TIMEOUT_US);
}


status_t
HyperVKeyboard::_SetIndicators(led_info ledInfo)
{
	hv_keyboard_msg_set_indicators message;
	message.header.type = HV_KEYBOARD_MSGTYPE_SET_INDICATORS;
	message.indicators = 0;

	if (ledInfo.num_lock)
		message.indicators |= HV_KEYBOARD_INDICATORS_NUM_LOCK;
	if (ledInfo.caps_lock)
		message.indicators |= HV_KEYBOARD_INDICATORS_CAPS_LOCK;
	if (ledInfo.scroll_lock)
		message.indicators |= HV_KEYBOARD_INDICATORS_SCROLL_LOCK;

	TRACE("Set indicators 0x%X\n", message.indicators);
	return fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, &message,
		sizeof(message), true, (uint64)&message);
}


bool
HyperVKeyboard::_CanWriteKeystroke()
{
	MutexLocker locker(fKeyBufferLock);
	return ring_buffer_writable(fKeyBuffer) >= sizeof(raw_key_info);
}


void
HyperVKeyboard::_WriteKeystroke(raw_key_info* keyInfo)
{
	// Wait for buffer to be available
	for (uint32 i = 0; i < 1000; i++) {
		if (_CanWriteKeystroke()) {
			MutexLocker locker(fKeyBufferLock);
			ring_buffer_write(fKeyBuffer, reinterpret_cast<uint8*>(keyInfo), sizeof(*keyInfo));
			locker.Unlock();

			release_sem_etc(fKeyBufferSem, 1, B_DO_NOT_RESCHEDULE);
			return;
		}

		snooze(HV_MS_TO_US(50));
	}

	ERROR("Keystroke buffer full, discarding\n");
}


status_t
HyperVKeyboard::_ReadKeystroke(raw_key_info* keyInfo)
{
	status_t status = acquire_sem(fKeyBufferSem);
	if (status != B_OK)
		return status;

	MutexLocker locker(fKeyBufferLock);
	if (ring_buffer_read(fKeyBuffer, reinterpret_cast<uint8*>(keyInfo), sizeof(*keyInfo)) == 0)
		return B_BUSY;

	return B_OK;
}
