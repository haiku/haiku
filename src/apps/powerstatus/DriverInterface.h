/*
 * Copyright 2009-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */
#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H


#include <Handler.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include "device/power_managment.h"


typedef BObjectList<BHandler> WatcherList;

const uint32 kMsgUpdate = 'updt';


struct battery_info {
	int8		state;
	int32		capacity;
	int32		full_capacity;
	time_t		time_left;
	int32		current_rate;
};


/*! Handle a list of watcher and broadcast a messages to them. */
class Monitor {
public:
	virtual						~Monitor();

	virtual status_t			StartWatching(BHandler* target);
	virtual status_t			StopWatching(BHandler* target);

	virtual void				Broadcast(uint32 message);

protected:
	WatcherList					fWatcherList;
};


class PowerStatusDriverInterface : public Monitor, public BReferenceable {
public:
								PowerStatusDriverInterface();
								~PowerStatusDriverInterface();

	virtual status_t			StartWatching(BHandler* target);
	virtual status_t			StopWatching(BHandler* target);
	virtual void				Broadcast(uint32 message);

	virtual status_t			Connect() = 0;
	virtual void				Disconnect();

	virtual status_t 			GetBatteryInfo(int32 index,
									battery_info* status) = 0;
	virtual status_t 			GetExtendedBatteryInfo(int32 index,
									acpi_extended_battery_info* info) = 0;

	virtual int32				GetBatteryCount() = 0;

protected:
	virtual void				_WatchPowerStatus() = 0;

protected:
	int32						fIsWatching;
	sem_id						fWaitSem;

private:
	static int32				_ThreadWatchPowerFunction(void* data);

	thread_id					fThread;
	BLocker						fListLocker;
};


#endif	// DRIVER_INTERFACE_H
