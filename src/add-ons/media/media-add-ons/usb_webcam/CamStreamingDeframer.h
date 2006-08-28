#ifndef _CAM_STREAMING_DEFRAMER_H
#define _CAM_STREAMING_DEFRAMER_H

#include "CamDeframer.h"

class CamStreamingDeframer : public CamDeframer 
{
public:
			CamStreamingDeframer(CamDevice *device);
virtual 	~CamStreamingDeframer();
					// BPositionIO interface
					// write from usb transfers
virtual ssize_t		Write(const void *buffer, size_t size);

private:

BMallocIO	fInputBuff;


};


#endif /* _CAM_STREAMING_DEFRAMER_H */
