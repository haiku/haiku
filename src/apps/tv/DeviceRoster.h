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

#ifndef __DEVICE_ROSTER_H
#define __DEVICE_ROSTER_H

#include <MediaRoster.h>

class DeviceRoster
{
public:
						DeviceRoster();
						~DeviceRoster();
						
	int					DeviceCount();

	const char *		DeviceName(int i);
	media_node			DeviceNode(int i);
	
	bool				IsRawAudioOutputFree(int i);
	bool				IsEncAudioOutputFree(int i);
	bool				IsRawVideoOutputFree(int i);
	bool				IsEncVideoOutputFree(int i);

public:
	BMediaRoster*		MediaRoster();

private:						
	enum { MAX_DEVICE_COUNT = 8 };

	class device_info
	{
	public:
		media_node	node;
		char 		name[B_MEDIA_NAME_LENGTH];
	};

private:	
	device_info		fDeviceInfo[MAX_DEVICE_COUNT];
	int				fDeviceCount;
};

extern DeviceRoster *gDeviceRoster;

#endif
