/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_FILTER_INTERFACE_H
#define _CAM_FILTER_INTERFACE_H

#include <kernel/OS.h>
#include <support/DataIO.h>
#include <interface/Rect.h>
class CamDevice;

class CamFilterInterface : public BPositionIO {
public:
			CamFilterInterface(CamDevice *device);
virtual 	~CamFilterInterface();

	// filter chain handling, should be accessed with device locked
					// stack up this filter on top of 'to'
status_t			ChainFilter(CamFilterInterface *to);
					// break the chain
status_t			DetachFilter(CamFilterInterface *from);
					// accessor (next is actually processing data before self)
CamFilterInterface	*ChainFilter();


	// BPositionIO interface
virtual ssize_t		Read(void *buffer, size_t size);
virtual ssize_t		ReadAt(off_t pos, void *buffer, size_t size);

virtual ssize_t		Write(const void *buffer, size_t size);
virtual ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual off_t		Seek(off_t position, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);
	// size of the buffer required for reading a whole frame
virtual size_t		FrameSize();

	// frame handling
virtual status_t	WaitFrame(bigtime_t timeout);
virtual status_t	DropFrame();
	// video settings propagation
virtual status_t	SetVideoFrame(BRect frame);


protected:

CamDevice	*fDevice;
CamFilterInterface	*fNextOfKin;
BRect		fVideoFrame;
};


#endif /* _CAM_FILTER_INTERFACE_H */
