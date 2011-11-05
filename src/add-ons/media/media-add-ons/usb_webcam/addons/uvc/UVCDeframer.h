/*
 * Copyright 2011, Gabriel Hartmann, gabriel.hartmann@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UVC_DEFRAMER_H
#define _UVC_DEFRAMER_H


#include "CamDeframer.h"

#include <USB3.h>


class UVCDeframer : public CamDeframer {
public:
								UVCDeframer(CamDevice *device);
	virtual 					~UVCDeframer();
					// BPositionIO interface
					// write from usb transfers
	virtual ssize_t				Write(const void *buffer, size_t size);

private:
	void						_PrintBuffer(const void* buffer, size_t size);

	int32						fFrameCount;
	int32						fID;
	BMallocIO					fInputBuffer;
};

#endif /* _UVC_DEFRAMER_H */

