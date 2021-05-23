/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioInputDevice.h"

#include <virtio_input_driver.h>
#include <virtio_defs.h>

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <String.h>


//#define TRACE_VIRTIO_INPUT_DEVICE
#ifdef TRACE_VIRTIO_INPUT_DEVICE
#       define TRACE(x...) debug_printf("virtio_input_device: " x)
#else
#       define TRACE(x...) ;
#endif
#define ERROR(x...) debug_printf("virtio_input_device: " x)
#define CALLED() TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


enum {
	kWatcherThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4,
};


template<typename Type>
inline static void SetBit(Type &val, int bit) {val |= Type(1) << bit;}

template<typename Type>
inline static void ClearBit(Type &val, int bit) {val &= ~(Type(1) << bit);}

template<typename Type>
inline static void InvertBit(Type &val, int bit) {val ^= Type(1) << bit;}

template<typename Type>
inline static void SetBitTo(Type &val, int bit, bool isSet) {
	val ^= ((isSet? -1: 0) ^ val) & (Type(1) << bit);}

template<typename Type>
inline static bool IsBitSet(Type val, int bit) {
	return (val & (Type(1) << bit)) != 0;}


#ifdef TRACE_VIRTIO_INPUT_DEVICE
static void WriteInputPacket(const VirtioInputPacket &pkt)
{
	switch (pkt.type) {
		case kVirtioInputEvSyn:
			TRACE("syn");
			break;
		case kVirtioInputEvKey:
			TRACE("key, ");
			switch (pkt.code) {
				case kVirtioInputBtnLeft:
					TRACE("left");
					break;
				case kVirtioInputBtnRight:
					TRACE("middle");
					break;
				case kVirtioInputBtnMiddle:
					TRACE("right");
					break;
				case kVirtioInputBtnGearDown:
					TRACE("gearDown");
					break;
				case kVirtioInputBtnGearUp:
					TRACE("gearUp");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvRel:
			TRACE("rel, ");
			switch (pkt.code) {
				case kVirtioInputRelX:
					TRACE("relX");
					break;
				case kVirtioInputRelY:
					TRACE("relY");
					break;
				case kVirtioInputRelZ:
					TRACE("relZ");
					break;
				case kVirtioInputRelWheel:
					TRACE("relWheel");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvAbs:
			TRACE("abs, ");
			switch (pkt.code) {
				case kVirtioInputAbsX:
					TRACE("absX");
					break;
				case kVirtioInputAbsY:
					TRACE("absY");
					break;
				case kVirtioInputAbsZ:
					TRACE("absZ");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvRep:
			TRACE("rep");
			break;
		default:
			TRACE("?(%d)", pkt.type);
	}
	switch (pkt.type) {
		case kVirtioInputEvSyn:
			break;
		case kVirtioInputEvKey:
			TRACE(", ");
			if (pkt.value == 0) {
				TRACE("up");
			} else if (pkt.value == 1) {
				TRACE("down");
			} else {
				TRACE("%d", pkt.value);
			}
			break;
		default:
			TRACE(", ");
			TRACE("%d", pkt.value);
	}
}
#endif /* TRACE_VIRTIO_INPUT_DEVICE */


//#pragma mark VirtioInputDevice


VirtioInputDevice::VirtioInputDevice()
{
}

VirtioInputDevice::~VirtioInputDevice()
{
}


status_t
VirtioInputDevice::InitCheck()
{
	static input_device_ref *devices[3];
	input_device_ref **devicesEnd = devices;

	FileDescriptorCloser fd;

	// TODO: dynamically scan and detect device type

	ObjectDeleter<VirtioInputHandler> tablet(
		new TabletHandler(this, "VirtIO tablet"));
	fd.SetTo(open("/dev/input/virtio/0/raw", O_RDWR));
	if (fd.IsSet()) {
		tablet->SetFd(fd.Detach());
		*devicesEnd++ = tablet->Ref();
		tablet.Detach();
	} else {
		TRACE("Unable to detect tablet device!");
	}

	ObjectDeleter<VirtioInputHandler> keyboard(
		new KeyboardHandler(this, "VirtIO keyboard"));
	fd.SetTo(open("/dev/input/virtio/1/raw", O_RDWR));
	if (fd.IsSet()) {
		keyboard->SetFd(fd.Detach());
		*devicesEnd++ = keyboard->Ref();
		keyboard.Detach();
	} else {
		TRACE("Unable to detect keyboard device!");
	}

	*devicesEnd = NULL;

	RegisterDevices(devices);
	return B_OK;
}


status_t
VirtioInputDevice::Start(const char* name, void* cookie)
{
	return ((VirtioInputHandler*)cookie)->Start();
}


status_t
VirtioInputDevice::Stop(const char* name, void* cookie)
{
	return ((VirtioInputHandler*)cookie)->Stop();
}


status_t
VirtioInputDevice::Control(const char* name, void* cookie, uint32 command,
	BMessage* message)
{
	return ((VirtioInputHandler*)cookie)->Control(command, message);
}


//#pragma mark VirtioInputHandler


VirtioInputHandler::VirtioInputHandler(VirtioInputDevice* dev, const char* name,
	input_device_type type)
	:
	fDev(dev),
	fWatcherThread(B_ERROR),
	fRun(false)
{
	fRef.name = (char*)name; // NOTE: name should be constant data
	fRef.type = type;
	fRef.cookie = this;
}


VirtioInputHandler::~VirtioInputHandler()
{

}


void
VirtioInputHandler::SetFd(int fd)
{
	fDeviceFd.SetTo(fd);
}


status_t
VirtioInputHandler::Start()
{
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fRef.name);

	if (fWatcherThread < 0) {
		fWatcherThread = spawn_thread(Watcher, threadName,
			kWatcherThreadPriority, this);

		if (fWatcherThread < B_OK)
			return fWatcherThread;

		fRun = true;
		resume_thread(fWatcherThread);
	}
	return B_OK;
}


status_t
VirtioInputHandler::Stop()
{
	// TODO: Use condition variable to sync access? suspend_thread
	// avoids a race condition so it doesn't exit before wait_for_thread

	if (fWatcherThread >= B_OK) {
		// ioctl(fDeviceFd.Get(), virtioInputCancelIO, NULL, 0);
		suspend_thread(fWatcherThread);
		fRun = false;
		status_t res;
		wait_for_thread(fWatcherThread, &res);
		fWatcherThread = B_ERROR;
	}
	return B_OK;
}


status_t
VirtioInputHandler::Control(uint32 command, BMessage* message)
{
	return B_OK;
}


int32
VirtioInputHandler::Watcher(void *arg)
{
	VirtioInputHandler &handler = *((VirtioInputHandler*)arg);
	handler.Reset();
	while (handler.fRun) {
		VirtioInputPacket pkt;
		status_t res = ioctl(handler.fDeviceFd.Get(), virtioInputRead, &pkt,
			sizeof(pkt));
		// if (res == B_CANCELED) return B_OK;
		if (res < B_OK)
			continue;
		handler.PacketReceived(pkt);
	}
	return B_OK;
}


//#pragma mark KeyboardHandler


KeyboardHandler::KeyboardHandler(VirtioInputDevice* dev, const char* name)
	:
	VirtioInputHandler(dev, name, B_KEYBOARD_DEVICE),
	fRepeatThread(-1),
	fRepeatThreadSem(-1)
{
	TRACE("+KeyboardHandler()\n");
	{
		// TODO: Similar to B_KEY_MAP_CHANGED below?
		key_map *keyMap = NULL;
		char *chars = NULL;
		get_key_map(&keyMap, &chars);
		fKeyMap.SetTo(keyMap);
		fChars.SetTo(chars);
	}
	TRACE("  fKeymap: %p\n", fKeyMap.Get());
	TRACE("  fChars: %p\n", fChars.Get());
	get_key_repeat_delay(&fRepeatDelay);
	get_key_repeat_rate (&fRepeatRate);
	TRACE("  fRepeatDelay: %" B_PRIdBIGTIME "\n", fRepeatDelay);
	TRACE("  fRepeatRate: % " B_PRId32 "\n", fRepeatRate);

	if (fRepeatRate < 1)
		fRepeatRate = 1;
}


KeyboardHandler::~KeyboardHandler()
{
	_StopRepeating();
}


void
KeyboardHandler::Reset()
{
	memset(&fNewState, 0, sizeof(KeyboardState));
	memcpy(&fState, &fNewState, sizeof(KeyboardState));
	_StopRepeating();
}


status_t
KeyboardHandler::Control(uint32 command, BMessage* message)
{
	switch (command) {
		case B_KEY_MAP_CHANGED: {
			key_map *keyMap = NULL;
			char *chars = NULL;
			get_key_map(&keyMap, &chars);
			if (keyMap == NULL || chars == NULL)
				return B_NO_MEMORY;
			fKeyMap.SetTo(keyMap);
			fChars.SetTo(chars);
			return B_OK;
		}
		case B_KEY_REPEAT_DELAY_CHANGED:
			get_key_repeat_delay(&fRepeatDelay);
			TRACE("  fRepeatDelay: %" B_PRIdBIGTIME "\n", fRepeatDelay);
			return B_OK;
		case B_KEY_REPEAT_RATE_CHANGED:
			get_key_repeat_rate(&fRepeatRate);
			TRACE("  fRepeatRate: %" B_PRId32 "\n", fRepeatRate);
			if (fRepeatRate < 1) fRepeatRate = 1;
			return B_OK;
	}
	return VirtioInputHandler::Control(command, message);
}


void
KeyboardHandler::PacketReceived(const VirtioInputPacket &pkt)
{
#ifdef TRACE_VIRTIO_INPUT_DEVICE
	TRACE("keyboard: ");
	WriteInputPacket(pkt);
	TRACE("\n");
#endif
	switch (pkt.type) {
		case kVirtioInputEvKey: {
			if (pkt.code < 256)
				SetBitTo(fNewState.keys[pkt.code / 8], pkt.code % 8,
					pkt.value != 0);
			break;
		}
		case kVirtioInputEvSyn: {
			fState.when = system_time();
			_StateChanged();
		}
	}
}


bool
KeyboardHandler::_IsKeyPressed(const KeyboardState &state, uint32 key)
{
	return key < 256 && IsBitSet(state.keys[key / 8], key % 8);
}


void
KeyboardHandler::_KeyString(uint32 code, char *str, size_t len)
{
	char *ch;
	switch (fNewState.modifiers & (
		B_SHIFT_KEY | B_CONTROL_KEY | B_OPTION_KEY | B_CAPS_LOCK)) {
		case B_OPTION_KEY | B_CAPS_LOCK | B_SHIFT_KEY:
			ch = fChars.Get() + fKeyMap->option_caps_shift_map[code];
			break;
		case B_OPTION_KEY | B_CAPS_LOCK:
			ch = fChars.Get() + fKeyMap->option_caps_map[code];
			break;
		case B_OPTION_KEY | B_SHIFT_KEY:
			ch = fChars.Get() + fKeyMap->option_shift_map[code];
			break;
		case B_OPTION_KEY:
			ch = fChars.Get() + fKeyMap->option_map[code];
			break;
		case B_CAPS_LOCK  | B_SHIFT_KEY:
			ch = fChars.Get() + fKeyMap->caps_shift_map[code];
			break;
		case B_CAPS_LOCK:
			ch = fChars.Get() + fKeyMap->caps_map[code];
			break;
		case B_SHIFT_KEY:
			ch = fChars.Get() + fKeyMap->shift_map[code];
			break;
		default:
			if ((fNewState.modifiers & B_CONTROL_KEY) != 0)
				ch = fChars.Get() + fKeyMap->control_map[code];
			else
				ch = fChars.Get() + fKeyMap->normal_map[code];
	}
	if (len > 0) {
		uint32 i;
		for (i = 0; (i < (uint32)ch[0]) && (i < len - 1); i++)
			str[i] = ch[i + 1];
		str[i] = '\0';
	}
}


void
KeyboardHandler::_StartRepeating(BMessage* msg)
{
	if (fRepeatThread >= B_OK)
		_StopRepeating();

	fRepeatMsg = *msg;
	fRepeatThread = spawn_thread(_RepeatThread, "repeat thread",
		B_REAL_TIME_DISPLAY_PRIORITY + 4, this);
	fRepeatThreadSem = create_sem(0, "repeat thread sem");
	if (fRepeatThread >= B_OK)
		resume_thread(fRepeatThread);
}


void
KeyboardHandler::_StopRepeating()
{
	if (fRepeatThread >= B_OK) {
		status_t res;
		release_sem(fRepeatThreadSem);
		wait_for_thread(fRepeatThread, &res);
		fRepeatThread = -1;
		delete_sem(fRepeatThreadSem);
		fRepeatThreadSem = -1;
	}
}


status_t
KeyboardHandler::_RepeatThread(void *arg)
{
	status_t res;
	KeyboardHandler *h = (KeyboardHandler*)arg;

	res = acquire_sem_etc(h->fRepeatThreadSem, 1, B_RELATIVE_TIMEOUT,
		h->fRepeatDelay);
	if (res >= B_OK)
		return B_OK;

	while (true) {
		int32 count;

		h->fRepeatMsg.ReplaceInt64("when", system_time());
		h->fRepeatMsg.FindInt32("be:key_repeat", &count);
		h->fRepeatMsg.ReplaceInt32("be:key_repeat", count + 1);

		ObjectDeleter<BMessage> msg(new(std::nothrow) BMessage(h->fRepeatMsg));
		if (msg.IsSet() && h->Device()->EnqueueMessage(msg.Get()) >= B_OK)
			msg.Detach();

		res = acquire_sem_etc(h->fRepeatThreadSem, 1, B_RELATIVE_TIMEOUT,
			(bigtime_t)10000000 / h->fRepeatRate);
		if (res >= B_OK)
			return B_OK;
	}
}


void
KeyboardHandler::_StateChanged()
{
	uint32 i, j;

	fNewState.modifiers = fState.modifiers
		& (B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
	if (_IsKeyPressed(fNewState, fKeyMap->left_shift_key))
		fNewState.modifiers |= B_SHIFT_KEY   | B_LEFT_SHIFT_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->right_shift_key))
		fNewState.modifiers |= B_SHIFT_KEY   | B_RIGHT_SHIFT_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->left_command_key))
		fNewState.modifiers |= B_COMMAND_KEY | B_LEFT_COMMAND_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->right_command_key))
		fNewState.modifiers |= B_COMMAND_KEY | B_RIGHT_COMMAND_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->left_control_key))
		fNewState.modifiers |= B_CONTROL_KEY | B_LEFT_CONTROL_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->right_control_key))
		fNewState.modifiers |= B_CONTROL_KEY | B_RIGHT_CONTROL_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->caps_key))
		fNewState.modifiers ^= B_CAPS_LOCK;
	if (_IsKeyPressed(fNewState, fKeyMap->scroll_key))
		fNewState.modifiers ^= B_SCROLL_LOCK;
	if (_IsKeyPressed(fNewState, fKeyMap->num_key))
		fNewState.modifiers ^= B_NUM_LOCK;
	if (_IsKeyPressed(fNewState, fKeyMap->left_option_key))
		fNewState.modifiers |= B_OPTION_KEY  | B_LEFT_OPTION_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->right_option_key))
		fNewState.modifiers |= B_OPTION_KEY  | B_RIGHT_OPTION_KEY;
	if (_IsKeyPressed(fNewState, fKeyMap->menu_key))
		fNewState.modifiers |= B_MENU_KEY;

	if (fState.modifiers != fNewState.modifiers) {
		ObjectDeleter<BMessage> msg(
			new(std::nothrow) BMessage(B_MODIFIERS_CHANGED));
		if (msg.IsSet()) {
			msg->AddInt64("when", system_time());
			msg->AddInt32("modifiers", fNewState.modifiers);
			msg->AddInt32("be:old_modifiers", fState.modifiers);
			msg->AddData("states", B_UINT8_TYPE, fNewState.keys, 16);

			if (Device()->EnqueueMessage(msg.Get()) >= B_OK) {
				msg.Detach();
				fState.modifiers = fNewState.modifiers;
			}
		}
	}


	uint8 diff[16];
	char rawCh;
	char str[5];

	for (i = 0; i < 16; ++i)
		diff[i] = fState.keys[i] ^ fNewState.keys[i];

	for (i = 0; i < 128; ++i) {
		if (diff[i/8] & (1 << (i % 8))) {
			ObjectDeleter<BMessage> msg(new(std::nothrow) BMessage());
			if (msg.IsSet()) {
				_KeyString(i, str, sizeof(str));

				msg->AddInt64("when", system_time());
				msg->AddInt32("key", i);
				msg->AddInt32("modifiers", fNewState.modifiers);
				msg->AddData("states", B_UINT8_TYPE, fNewState.keys, 16);

				if (str[0] != '\0') {
					if (fChars.Get()[fKeyMap->normal_map[i]] != 0)
						rawCh = fChars.Get()[fKeyMap->normal_map[i] + 1];
					else
						rawCh = str[0];

					for (j = 0; str[j] != '\0'; ++j)
						msg->AddInt8("byte", str[j]);

					msg->AddString("bytes", str);
					msg->AddInt32("raw_char", rawCh);
				}

				if (fNewState.keys[i / 8] & (1 << (i % 8))) {
					if (str[0] != '\0')
						msg->what = B_KEY_DOWN;
					else
						msg->what = B_UNMAPPED_KEY_DOWN;

					msg->AddInt32("be:key_repeat", 1);
					_StartRepeating(msg.Get());
				} else {
					if (str[0] != '\0')
						msg->what = B_KEY_UP;
					else
						msg->what = B_UNMAPPED_KEY_UP;

					_StopRepeating();
				}

				if (Device()->EnqueueMessage(msg.Get()) >= B_OK) {
					msg.Detach();
					for (j = 0; j < 16; ++j)
						fState.keys[j] = fNewState.keys[j];
				}
			}
		}
	}
}


//#pragma mark TabletHandler


TabletHandler::TabletHandler(VirtioInputDevice* dev, const char* name)
	:
	VirtioInputHandler(dev, name, B_POINTING_DEVICE)
{
}


void
TabletHandler::Reset()
{
	memset(&fNewState, 0, sizeof(TabletState));
	fNewState.x = 0.5f;
	fNewState.y = 0.5f;
	memcpy(&fState, &fNewState, sizeof(TabletState));
	fLastClick = -1;
	fLastClickBtn = -1;

	get_click_speed(&fClickSpeed);
	TRACE("  fClickSpeed: %" B_PRIdBIGTIME "\n", fClickSpeed);
}


status_t
TabletHandler::Control(uint32 command, BMessage* message)
{
	switch (command) {
		case B_CLICK_SPEED_CHANGED: {
			get_click_speed(&fClickSpeed);
			TRACE("  fClickSpeed: %" B_PRIdBIGTIME "\n", fClickSpeed);
			return B_OK;
		}
	}
	return VirtioInputHandler::Control(command, message);
}


void
TabletHandler::PacketReceived(const VirtioInputPacket &pkt)
{
	switch (pkt.type) {
		case kVirtioInputEvAbs: {
			switch (pkt.code) {
				case kVirtioInputAbsX:
					fNewState.x = float(pkt.value) / 32768.0f;
					break;
				case kVirtioInputAbsY:
					fNewState.y = float(pkt.value) / 32768.0f;
					break;
			}
			break;
		}
		case kVirtioInputEvRel: {
			switch (pkt.code) {
				case kVirtioInputRelWheel:
					fNewState.wheelY -= pkt.value;
					break;
			}
			break;
		}
		case kVirtioInputEvKey: {
			switch (pkt.code) {
				case kVirtioInputBtnLeft:
					SetBitTo(fNewState.buttons, 0, pkt.value != 0);
					break;
				case kVirtioInputBtnRight:
					SetBitTo(fNewState.buttons, 1, pkt.value != 0);
					break;
				case kVirtioInputBtnMiddle:
					SetBitTo(fNewState.buttons, 2, pkt.value != 0);
					break;
			}
			break;
		}
		case kVirtioInputEvSyn: {
			fState.when = system_time();

			// update pos
			if (fState.x != fNewState.x || fState.y != fNewState.y
				|| fState.pressure != fNewState.pressure) {
				fState.x = fNewState.x;
				fState.y = fNewState.y;
				fState.pressure = fNewState.pressure;
				ObjectDeleter<BMessage> msg(
					new(std::nothrow) BMessage(B_MOUSE_MOVED));
				if (!msg.IsSet() || !_FillMessage(*msg.Get(), fState))
					return;

				if (Device()->EnqueueMessage(msg.Get()) >= B_OK)
					msg.Detach();
			}

			// update buttons
			for (int i = 0; i < 32; i++) {
				if ((IsBitSet(fState.buttons, i)
					!= IsBitSet(fNewState.buttons, i))) {
					InvertBit(fState.buttons, i);

					// TODO: new B_MOUSE_DOWN for every button clicked together?
					// should be refactored to look like other input drivers.

					ObjectDeleter<BMessage> msg(new(std::nothrow) BMessage());
					if (!msg.IsSet() || !_FillMessage(*msg.Get(), fState))
						return;

					if (IsBitSet(fState.buttons, i)) {
						msg->what = B_MOUSE_DOWN;
						if (i == fLastClickBtn
							&& fState.when - fLastClick <= fClickSpeed)
							fState.clicks++;
						else
							fState.clicks = 1;
						fLastClickBtn = i;
						fLastClick = fState.when;
						msg->AddInt32("clicks", fState.clicks);
					} else
						msg->what = B_MOUSE_UP;

					if (Device()->EnqueueMessage(msg.Get()) >= B_OK)
						msg.Detach();
				}
			}

			// update wheel
			if (fState.wheelX != fNewState.wheelX
				|| fState.wheelY != fNewState.wheelY) {
				ObjectDeleter<BMessage> msg(
					new(std::nothrow) BMessage(B_MOUSE_WHEEL_CHANGED));
				if (!msg.IsSet()
					|| msg->AddInt64("when", fState.when) < B_OK
					|| msg->AddFloat("be:wheel_delta_x",
						fNewState.wheelX - fState.wheelX) < B_OK
					|| msg->AddFloat("be:wheel_delta_y",
						fNewState.wheelY - fState.wheelY) < B_OK) {
					return;
				}

				fState.wheelX = fNewState.wheelX;
				fState.wheelY = fNewState.wheelY;
				if (Device()->EnqueueMessage(msg.Get()) >= B_OK)
					msg.Detach();
			}
			break;
		}
	}
}


bool
TabletHandler::_FillMessage(BMessage &msg, const TabletState &s)
{
	if (msg.AddInt64("when", s.when) < B_OK
		|| msg.AddInt32("buttons", s.buttons) < B_OK
		|| msg.AddFloat("x", s.x) < B_OK
		|| msg.AddFloat("y", s.y) < B_OK) {
		return false;
	}
	msg.AddFloat("be:tablet_x", s.x);
	msg.AddFloat("be:tablet_y", s.y);
	msg.AddFloat("be:tablet_pressure", s.pressure);
	return true;
}


//#pragma mark -


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) VirtioInputDevice();
}
