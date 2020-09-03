/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INPUT_SERVER_APP_H
#define INPUT_SERVER_APP_H


#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#define DEBUG 1

#include <Application.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <ObjectList.h>
#include <OS.h>
#include <Screen.h>
#include <SupportDefs.h>

#include <shared_cursor_area.h>

#include "AddOnManager.h"
#include "KeyboardSettings.h"
#include "MouseSettings.h"
#include "PathList.h"


#define INPUTSERVER_SIGNATURE "application/x-vnd.Be-input_server"
	// use this when target should replace R5 input_server

typedef BObjectList<BMessage> EventList;

class BottomlineWindow;

class InputDeviceListItem {
	public:
		InputDeviceListItem(BInputServerDevice& serverDevice,
			const input_device_ref& device);
		~InputDeviceListItem();

		void Start();
		void Stop();
		void Control(uint32 code, BMessage* message);

		const char* Name() const { return fDevice.name; }
		input_device_type Type() const { return fDevice.type; }
		bool Running() const { return fRunning; }

		bool HasName(const char* name) const;
		bool HasType(input_device_type type) const;
		bool Matches(const char* name, input_device_type type) const;

		BInputServerDevice* ServerDevice() { return fServerDevice; }

	private:
		BInputServerDevice* fServerDevice;
		input_device_ref   	fDevice;
		bool 				fRunning;
};

namespace BPrivate {

class DeviceAddOn {
public:
								DeviceAddOn(BInputServerDevice* device);
								~DeviceAddOn();

			bool				HasPath(const char* path) const;
			status_t			AddPath(const char* path);
			status_t			RemovePath(const char* path);
			int32				CountPaths() const;

			BInputServerDevice*	Device() { return fDevice; }

private:
			BInputServerDevice*	fDevice;
			PathList			fMonitoredPaths;
};

}	// namespace BPrivate

class _BMethodAddOn_ {
	public:
		_BMethodAddOn_(BInputServerMethod *method, const char* name,
			const uchar* icon);
		~_BMethodAddOn_();

		status_t SetName(const char* name);
		status_t SetIcon(const uchar* icon);
		status_t SetMenu(const BMenu* menu, const BMessenger& messenger);
		status_t MethodActivated(bool activate);
		status_t AddMethod();
		int32 Cookie() { return fCookie; }

	private:
		BInputServerMethod* fMethod;
		char* fName;
		uchar fIcon[16*16*1];
		const BMenu* fMenu;
		BMessenger fMessenger;
		int32 fCookie;
};

class KeymapMethod : public BInputServerMethod {
	public:
		KeymapMethod();
		~KeymapMethod();
};

class InputServer : public BApplication {
	public:
		InputServer();
		virtual ~InputServer();

		virtual void ArgvReceived(int32 argc, char** argv);

		virtual bool QuitRequested();
		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage* message);

		void HandleSetMethod(BMessage* message);
		status_t HandleGetSetMouseType(BMessage* message, BMessage* reply);
		status_t HandleGetSetMouseAcceleration(BMessage* message, BMessage* reply);
		status_t HandleGetSetKeyRepeatDelay(BMessage* message, BMessage* reply);
		status_t HandleGetKeyInfo(BMessage* message, BMessage* reply);
		status_t HandleGetModifiers(BMessage* message, BMessage* reply);
		status_t HandleGetModifierKey(BMessage* message, BMessage* reply);
		status_t HandleSetModifierKey(BMessage* message, BMessage* reply);
		status_t HandleSetKeyboardLocks(BMessage* message, BMessage* reply);
		status_t HandleGetSetMouseSpeed(BMessage* message, BMessage* reply);
		status_t HandleSetMousePosition(BMessage* message, BMessage* reply);
		status_t HandleGetSetMouseMap(BMessage* message, BMessage* reply);
		status_t HandleGetSetKeyboardID(BMessage* message, BMessage* reply);
		status_t HandleGetSetClickSpeed(BMessage* message, BMessage* reply);
		status_t HandleGetSetKeyRepeatRate(BMessage* message, BMessage* reply);
		status_t HandleGetSetKeyMap(BMessage* message, BMessage* reply);
		status_t HandleFocusUnfocusIMAwareView(BMessage* message, BMessage* reply);

		status_t EnqueueDeviceMessage(BMessage* message);
		status_t EnqueueMethodMessage(BMessage* message);
		status_t SetNextMethod(bool direction);
		void SetActiveMethod(BInputServerMethod* method);
		const BMessenger* MethodReplicant();
		void SetMethodReplicant(const BMessenger *replicant);
		bool EventLoopRunning();

		status_t GetDeviceInfo(const char* name, input_device_type *_type,
					bool *_isRunning = NULL);
		status_t GetDeviceInfos(BMessage *msg);
		status_t UnregisterDevices(BInputServerDevice& serverDevice,
					input_device_ref** devices = NULL);
		status_t RegisterDevices(BInputServerDevice& serverDevice,
					input_device_ref** devices);
		status_t StartStopDevices(const char* name, input_device_type type,
					bool doStart);
		status_t StartStopDevices(BInputServerDevice& serverDevice, bool start);
		status_t ControlDevices(const char *name, input_device_type type,
					uint32 code, BMessage* message);

		bool SafeMode();

		::AddOnManager* AddOnManager() { return fAddOnManager; }

		static BList gInputFilterList;
		static BLocker gInputFilterListLocker;

		static BList gInputMethodList;
		static BLocker gInputMethodListLocker;

		static KeymapMethod gKeymapMethod;

		BRect& ScreenFrame() { return fFrame; }

	private:
		typedef BApplication _inherited;

		status_t _LoadKeymap();
		status_t _LoadSystemKeymap();
		status_t _SaveKeymap(bool isDefault = false);
		void _InitKeyboardMouseStates();
		MouseSettings* _GetSettingsForMouse(BString mouseName);

		status_t _StartEventLoop();
		void _EventLoop();
		static status_t _EventLooper(void *arg);

		void _UpdateMouseAndKeys(EventList& events);
		bool _SanitizeEvents(EventList& events);
		bool _MethodizeEvents(EventList& events);
		bool _FilterEvents(EventList& events);
		void _DispatchEvents(EventList& events);

		void _FilterEvent(BInputServerFilter* filter, EventList& events,
					int32& index, int32& count);
		status_t _DispatchEvent(BMessage* event);

		status_t _AcquireInput(BMessage& message, BMessage& reply);
		void _ReleaseInput(BMessage* message);

	private:
		uint16			fKeyboardID;

		BList			fInputDeviceList;
		BLocker 		fInputDeviceListLocker;

		KeyboardSettings fKeyboardSettings;
		MultipleMouseSettings	fMouseSettings;

		BPoint			fMousePos;		// current mouse position
		key_info		fKeyInfo;		// current key info
		key_map			fKeys;			// current key_map
		char*			fChars;			// current keymap chars
		uint32			fCharsSize;		// current keymap char count

		port_id      	fEventLooperPort;

		::AddOnManager*	fAddOnManager;

		BScreen			fScreen;
		BRect			fFrame;

		BLocker			fEventQueueLock;
		EventList 		fEventQueue;

		BInputServerMethod*	fActiveMethod;
		EventList			fMethodQueue;
		const BMessenger*	fReplicantMessenger;
		BottomlineWindow*	fInputMethodWindow;
		bool				fInputMethodAware;

		sem_id 			fCursorSem;
		port_id			fAppServerPort;
		team_id			fAppServerTeam;
		area_id			fCursorArea;
		shared_cursor*	fCursorBuffer;

		typedef std::map<BString, MouseSettings*> mouse_settings_object;
		mouse_settings_object  fMouseSettingsObject;
};

extern InputServer* gInputServer;

#if DEBUG >= 1
#	if DEBUG == 2
#		undef PRINT
		inline void _iprint(const char *fmt, ...) {
			FILE* log = fopen("/var/log/input_server.log", "a");
			char buf[1024];
			va_list ap;
			va_start(ap, fmt);
			vsprintf(buf, fmt, ap);
			va_end(ap);
			fputs(buf, log);
			fflush(log);
			fclose(log);
        }
#		define PRINT(x)	_iprint x
#	else
#		undef PRINT
#		define PRINT(x)	SERIAL_PRINT(x)
#	endif
#	define PRINTERR(x)		PRINT(x)
#	define EXIT()          PRINT(("EXIT %s\n", __PRETTY_FUNCTION__))
#	define CALLED()        PRINT(("CALLED %s\n", __PRETTY_FUNCTION__))
#else
#	define EXIT()          ((void)0)
#	define CALLED()        ((void)0)
#	define PRINTERR(x)		SERIAL_PRINT(x)
#endif

#endif	/* INPUT_SERVER_APP_H */
