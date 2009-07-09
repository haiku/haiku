/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
 
#include "DriverInterface.h"

#include <stdio.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>


#define DRIVER_DIR "/dev/power"

CPUFreqDriverInterface::CPUFreqDriverInterface()
	:	fInitOK(false),
		fDriverHandler(-1),
		fIsWatching(false),
		fWatchingMessenger(NULL)
{
	fFrequencyStates = new StateList(20, true);
	
	if (_FindSpeedStepDriver(DRIVER_DIR) == B_OK)
		fInitOK = true;
	
	status_t ret;
	freq_info states[MAX_CPU_FREQUENCY_STATES];
	ret = ioctl(fDriverHandler, GET_CPU_FREQ_STATES, &states,
					sizeof(freq_info) * MAX_CPU_FREQUENCY_STATES);
	if (ret != B_OK)
		return;

	for (int i = 0; i < MAX_CPU_FREQUENCY_STATES && states[i].frequency != 0;
			i++) {
		fFrequencyStates->AddItem(new freq_info(states[i]));
	}
}


CPUFreqDriverInterface::~CPUFreqDriverInterface()
{
	StopWatching();
	delete fFrequencyStates;
	
	if (InitCheck() == B_OK)
		close(fDriverHandler);
}

								
status_t
CPUFreqDriverInterface::InitCheck()
{
	if (fInitOK)
		return B_OK;
	return B_ERROR;
}


StateList*
CPUFreqDriverInterface::GetCpuFrequencyStates()
{
	return fFrequencyStates;
}


freq_info*
CPUFreqDriverInterface::GetCurrentFrequencyState()
{
	uint16 stateId = 0;
	status_t ret;
	ret = ioctl(fDriverHandler, GET_CURENT_CPU_FREQ_STATE, &stateId,
					sizeof(uint16));
	if (ret != B_OK)
		return NULL;
	int32 i = 0;
	while (true) {
		freq_info* state = fFrequencyStates->ItemAt(i);
		if (!state)
			break;
		i++;
		
		if (state->id == stateId)
			return state;
	}
	return NULL;
}


int32
CPUFreqDriverInterface::GetNumberOfFrequencyStates()
{
	return fFrequencyStates->CountItems();
}


status_t
CPUFreqDriverInterface::SetFrequencyState(const freq_info* state)
{
	status_t ret;
	ret = ioctl(fDriverHandler, SET_CPU_FREQ_STATE, &(state->id),
					sizeof(uint16));
	return ret;
}


status_t
CPUFreqDriverInterface::StartWatching(BHandler* target)
{
	if (fIsWatching)
		return B_ERROR;
		
	if (fWatchingMessenger)
		delete fWatchingMessenger;
	fWatchingMessenger = new BMessenger(target);
	
	status_t status = B_ERROR;
	fThreadId = spawn_thread(&_ThreadWatchFreqFunction, "FreqThread",
								B_LOW_PRIORITY, this);
	if (fThreadId >= 0)
		status = resume_thread(fThreadId);
	else
		return fThreadId;

	if (status == B_OK) {
		fIsWatching = true;
		return B_OK;
	}
	return status;
}


status_t
CPUFreqDriverInterface::StopWatching()
{
	
	if (fIsWatching &&
			ioctl(fDriverHandler, STOP_WATCHING_CPU_FREQ) == B_OK)
	{
		status_t status;
		status = wait_for_thread(fThreadId, &status);
		
		delete fWatchingMessenger;
		fWatchingMessenger = NULL;

		fIsWatching = false;
	}
	
	return B_ERROR;
}


int32
CPUFreqDriverInterface::_ThreadWatchFreqFunction(void* data)
{
	CPUFreqDriverInterface* that = (CPUFreqDriverInterface*)data;
	that->_WatchFrequency();
	return 0;
}


void
CPUFreqDriverInterface::_WatchFrequency()
{
	uint16 newId = 0;
	while (ioctl(fDriverHandler, WATCH_CPU_FREQ, &newId,
					sizeof(uint16)) == B_OK)
	{		
		int i = 0;
		while (true) {
			freq_info* state = fFrequencyStates->ItemAt(i);
			if (!state)
				break;
			i++;
		
			if (state->id == newId) {
				BMessage msg(kMSGFrequencyChanged);
				msg.AddPointer("freq_info", state);
				fWatchingMessenger->SendMessage(&msg);
				break;
			}
		}
	}
}


status_t
CPUFreqDriverInterface::_FindSpeedStepDriver(const char* path)
{
	BDirectory dir(path);
	BEntry entry;
	
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		
		if (entry.IsDirectory()) {
			if (_FindSpeedStepDriver(path.Path()) == B_OK)
				return B_OK;
		}
		else {
			fDriverHandler = open(path.Path(), O_RDWR);
			if (fDriverHandler >= 0) {
				uint32 magicId = 0;
				status_t ret;
				ret = ioctl(fDriverHandler, IDENTIFY_DEVICE, &magicId,
								sizeof(uint32));
				if (ret == B_OK && magicId == kMagicFreqID)
					return B_OK;
				else
					close(fDriverHandler);
			}
		}
		
	}
	return B_ERROR;	
}
