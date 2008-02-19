/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef MASTER_SERVER_DEVICE_H
#define MASTER_SERVER_DEVICE_H

#include <stdio.h>

#include <add-ons/input_server/InputServerDevice.h>
#include <List.h>
#include <Locker.h>
#include <String.h>

// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 
class MasterServerDevice : public BInputServerDevice {
 public:
							MasterServerDevice();
	virtual					~MasterServerDevice();

							// BInputServerDevice
	virtual status_t		InitCheck();
	virtual status_t		SystemShuttingDown();

	virtual status_t		Start(const char* device, void* cookie);
	virtual	status_t		Stop(const char* device, void* cookie);
	virtual status_t		Control(const char	*device,
									void		*cookie,
									uint32		code, 
									BMessage	*message);

							// MasterServerDevice
			bigtime_t		DoubleClickSpeed() const
								{ return fDblClickSpeed; }
			const float*	AccelerationTable() const
								{ return fAccelerationTable; }		

private:
			void			_SearchDevices();

			void			_StopAll();
			void			_AddDevice(const char* path);
			void			_HandleNodeMonitor(BMessage* message);

			void			_CalculateAccelerationTable();

							// thread function for watching
							// the status of master device
//	static	int32			_ps2_disabler(void* cookie);
//			void			_StartPS2DisablerThread();
//			void			_StopPS2DisablerThread();

			bool			_LockDevices();
			void			_UnlockDevices();

			// list of mice objects
			BList			fDevices;
	volatile bool			fActive;

			// global stuff for all mice objects
			int32			fSpeed;
			int32			fAcceleration;
			bigtime_t		fDblClickSpeed;
			float			fAccelerationTable[256];

			// support halting the PS/2 mouse thread as long as we exist
			thread_id		fPS2DisablerThread;
			BLocker			fDeviceLock;
};

#ifndef DEBUG
#	define DEBUG 0
#endif

#if DEBUG
#	undef PRINT
	inline void _iprint(const char* fmt, ...) {
		FILE* log = fopen("/var/log/wacom.log", "a");
		va_list ap;
		va_start(ap, fmt);
		vfprintf(log, fmt, ap);
		va_end(ap);
		fflush(log);
		fclose(log);
       }
#	define PRINT(x)	_iprint x
#else
#	define PRINT(x)
#endif


#endif	// MASTER_SERVER_DEVICE_H
