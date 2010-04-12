/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_BUFFERING_DEFRAMER_H
#define _CAM_BUFFERING_DEFRAMER_H

#include "CamDeframer.h"

class CamBufferingDeframer : public CamDeframer {
public:
			CamBufferingDeframer(CamDevice *device);
virtual 	~CamBufferingDeframer();
					// BPositionIO interface
					// write from usb transfers
virtual ssize_t		Write(const void *buffer, size_t size);
size_t				DiscardFromInput(size_t size);

private:

BMallocIO	fInputBuffs[2];
int			fInputBuffIndex;


};


#endif /* _CAM_BUFFERING_DEFRAMER_H */
