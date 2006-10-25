/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H


#include "Keymap.h"
#include "TMWindow.h"
#include "kb_mouse_settings.h"

#include <InputServerDevice.h>
#include <List.h>

#include <stdio.h>


class KeyboardInputDevice;

struct keyboard_device {
	keyboard_device(const char *path);
	~keyboard_device();	
	
	KeyboardInputDevice *owner;
	input_device_ref device_ref;
	char path[B_PATH_NAME_LENGTH];
	int fd;
	thread_id device_watcher;
	kb_settings settings;
	volatile bool active;
	bool isAT;
	uint32 modifiers;
};


class KeyboardInputDevice : public BInputServerDevice {
	public:
		KeyboardInputDevice();
		~KeyboardInputDevice();

		virtual status_t InitCheck();

		virtual status_t Start(const char *name, void *cookie);
		virtual status_t Stop(const char *name, void *cookie);

		virtual status_t Control(const char *name, void *cookie,
			uint32 command, BMessage *message);

		virtual status_t SystemShuttingDown();

#ifdef DEBUG							 
		static FILE *sLogFile;
#endif

	private:
		status_t _HandleMonitor(BMessage *message);
		status_t _InitFromSettings(void *cookie, uint32 opcode = 0);
		void _RecursiveScan(const char *directory);

		status_t _AddDevice(const char *path);
		status_t _RemoveDevice(const char *path);

		status_t _EnqueueInlineInputMethod(int32 opcode, const char* string = NULL,
			bool confirmed = false, BMessage* keyDown = NULL);

		static int32 _DeviceWatcher(void *arg);

		void _SetLeds(keyboard_device *device);

		BList fDevices;
		Keymap	fKeymap;
		TMWindow *fTMWindow;
};

extern "C" BInputServerDevice *instantiate_input_device();

#if DEBUG
	inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
		fputs(buf, KeyboardInputDevice::sLogFile); fflush(KeyboardInputDevice::sLogFile); }
	#define LOG_ERR(text...) LOG(text)
#else
	#define LOG(text...)
	#define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

#endif	// KEYBOARD_INPUT_DEVICE_H
