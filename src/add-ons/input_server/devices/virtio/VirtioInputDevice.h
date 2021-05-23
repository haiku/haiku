/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIRTIOINPUTDEVICE_H_
#define _VIRTIOINPUTDEVICE_H_


#include <add-ons/input_server/InputServerDevice.h>
#include <AutoDeleter.h>
#include <Handler.h>
#include <InterfaceDefs.h>
#include <MessageRunner.h>


struct VirtioInputPacket;


struct KeyboardState {
	bigtime_t when;
	uint8 keys[256 / 8];
	uint32 modifiers;
};

struct TabletState {
	bigtime_t when;
	float x, y;
	float pressure;
	uint32 buttons;
	int32 clicks;
	int32 wheelX, wheelY;
};


class VirtioInputDevice : public BInputServerDevice
{
public:
							VirtioInputDevice();
	virtual					~VirtioInputDevice();

	virtual	status_t		InitCheck();

	virtual	status_t		Start(const char* name, void* cookie);
	virtual	status_t		Stop(const char* name, void* cookie);

	virtual	status_t		Control(const char* name, void* cookie,
								uint32 command, BMessage* message);
};


class VirtioInputHandler
{
public:
							VirtioInputHandler(VirtioInputDevice* dev,
								const char* name, input_device_type type);
	virtual					~VirtioInputHandler();
	inline	VirtioInputDevice*
							Device()
								{return fDev;}
	inline input_device_ref*
							Ref()
								{return &fRef;}
			void			SetFd(int fd);

			status_t		Start();
			status_t		Stop();

	static	status_t		Watcher(void* arg);

	virtual	void			Reset() = 0;
	virtual	status_t		Control(uint32 command, BMessage* message);
	virtual	void			PacketReceived(const VirtioInputPacket &pkt) = 0;

private:
			VirtioInputDevice*
							fDev;
			input_device_ref
							fRef;
			FileDescriptorCloser
							fDeviceFd;
			thread_id		fWatcherThread;
			bool			fRun;
};


class KeyboardHandler : public VirtioInputHandler
{
public:
							KeyboardHandler(VirtioInputDevice* dev,
								const char* name);
	virtual					~KeyboardHandler();

	virtual	void			Reset();
	virtual	status_t		Control(uint32 command, BMessage* message);
	virtual	void			PacketReceived(const VirtioInputPacket &pkt);

private:
	static	bool			_IsKeyPressed(const KeyboardState& state,
								uint32 key);
			void			_KeyString(uint32 code, char* str, size_t len);
			void			_StartRepeating(BMessage* msg);
			void			_StopRepeating();
	static	status_t		_RepeatThread(void* arg);
			void			_StateChanged();

private:
			KeyboardState	fState;
			KeyboardState	fNewState;
			BPrivate::AutoDeleter<key_map, BPrivate::MemoryDelete>
							fKeyMap;
			BPrivate::AutoDeleter<char, BPrivate::MemoryDelete>
							fChars;

			bigtime_t		fRepeatDelay;
			int32			fRepeatRate;
			thread_id		fRepeatThread;
			sem_id			fRepeatThreadSem;
			BMessage		fRepeatMsg;
};


class TabletHandler : public VirtioInputHandler
{
public:
							TabletHandler(VirtioInputDevice* dev,
								const char* name);

	virtual	void			Reset();
	virtual	status_t		Control(uint32 command, BMessage* message);
	virtual	void			PacketReceived(const VirtioInputPacket &pkt);

private:
	static	bool			_FillMessage(BMessage& msg, const TabletState& s);

private:
			TabletState		fState;
			TabletState		fNewState;
			bigtime_t		fLastClick;
			int				fLastClickBtn;

			bigtime_t		fClickSpeed;
};


extern "C" _EXPORT BInputServerDevice* instantiate_input_device();


#endif	// _VIRTIOINPUTDEVICE_H_
