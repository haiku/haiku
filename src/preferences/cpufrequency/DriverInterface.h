/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
 
#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Messenger.h>
#include <ObjectList.h>

#include "device/power_managment.h"

const uint32 kMSGFrequencyChanged = '&frc';

typedef BObjectList<freq_info> StateList;


class CPUFreqDriverInterface
{
	public:
								CPUFreqDriverInterface();
								~CPUFreqDriverInterface();
								
		status_t				InitCheck();
		StateList*				GetCpuFrequencyStates();
		freq_info*				GetCurrentFrequencyState();
		int32					GetNumberOfFrequencyStates();
		status_t				SetFrequencyState(const freq_info* state);
		
		status_t				StartWatching(BHandler* target);
		status_t				StopWatching();
	private:
		static int32			_ThreadWatchFreqFunction(void* data);
		void					_WatchFrequency();
		
		thread_id				fThreadId;
		status_t				_FindSpeedStepDriver(const char* path);
		
		bool					fInitOK;
		int32					fDriverHandler;
		
		StateList*				fFrequencyStates;
		
		bool					fIsWatching;
		BMessenger*				fWatchingMessenger;
};


#endif
