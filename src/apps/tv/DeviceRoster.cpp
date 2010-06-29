/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "DeviceRoster.h"

#include <stdio.h>
#include <string.h>
#include <Debug.h>


DeviceRoster *gDeviceRoster;



DeviceRoster::DeviceRoster()
{
	live_node_info info[MAX_DEVICE_COUNT];
	int32 info_count = MAX_DEVICE_COUNT;
	status_t err;
	err = MediaRoster()->GetLiveNodes(info, &info_count, NULL, NULL, "DVB*", 
		B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (err != B_OK || info_count < 1) { 
		printf("Can't find live DVB node. Found %ld nodes, error %08lx (%s)\n",
			info_count, err, strerror(err));
		fDeviceCount = 0;
	} else {
		fDeviceCount = info_count;
		for (int i = 0; i < fDeviceCount; i++) {
			fDeviceInfo[i].node = info[i].node;
			strcpy(fDeviceInfo[i].name, info[i].name);
		}
	}
}


DeviceRoster::~DeviceRoster()
{
}


int
DeviceRoster::DeviceCount()
{
	return fDeviceCount;
}


const char *
DeviceRoster::DeviceName(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);
	return fDeviceInfo[i].name;
}


media_node
DeviceRoster::DeviceNode(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);
	return fDeviceInfo[i].node;
}


bool
DeviceRoster::IsRawAudioOutputFree(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);

	media_output	output;
	int32			count;
	status_t		err;

	err = MediaRoster()->GetFreeOutputsFor(fDeviceInfo[i].node, &output, 1, 
		&count, B_MEDIA_RAW_AUDIO);
	return (err == B_OK) && (count == 1);
}


bool
DeviceRoster::IsEncAudioOutputFree(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);

	media_output	output;
	int32			count;
	status_t		err;

	err = MediaRoster()->GetFreeOutputsFor(fDeviceInfo[i].node, &output, 1, 
		&count, B_MEDIA_ENCODED_AUDIO);
	return (err == B_OK) && (count == 1);
}


bool
DeviceRoster::IsRawVideoOutputFree(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);

	media_output	output;
	int32			count;
	status_t		err;

	err = MediaRoster()->GetFreeOutputsFor(fDeviceInfo[i].node, &output, 1, 
		&count, B_MEDIA_RAW_VIDEO);
	return (err == B_OK) && (count == 1);
}


bool
DeviceRoster::IsEncVideoOutputFree(int i)
{
	ASSERT(i >= 0 && i < fDeviceCount);

	media_output	output;
	int32			count;
	status_t		err;

	err = MediaRoster()->GetFreeOutputsFor(fDeviceInfo[i].node, &output, 1, 
		&count, B_MEDIA_ENCODED_VIDEO);
	return (err == B_OK) && (count == 1);
}


BMediaRoster *
DeviceRoster::MediaRoster()
{
	return BMediaRoster::Roster();
}

