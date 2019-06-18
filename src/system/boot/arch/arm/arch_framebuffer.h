/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _ARCH_FRAMEBUFFER_H
#define _ARCH_FRAMEBUFFER_H


#include <boot/platform.h>
#include <SupportDefs.h>


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#   define TRACE(x...) dprintf(x)
#	define CALLED() dprintf("%s()\n", __func__);
#else
#   define TRACE(x...) ;
#	define CALLED() ;
#endif
#define ERROR(x...) dprintf(x)


class ArchFramebuffer {
public:
							ArchFramebuffer(addr_t base)
								:
								fBase(base) {};
							~ArchFramebuffer() {};

	virtual status_t		Init() { return B_OK; };
	virtual status_t		Probe() { return B_OK; };
	virtual status_t		SetDefaultMode() { return B_OK; };
	virtual status_t		SetVideoMode(int width, int height, int depth)
								{ return B_OK; };

	virtual addr_t			Base() { return fBase; };
			addr_t			PhysicalBase() { return fPhysicalBase; };
			size_t			Size() { return fSize; };

			int				Width() { return fCurrentWidth; };
			int				Height() { return fCurrentHeight; };
			int				Depth() { return fCurrentDepth; };
			int				BytesPerRow() { return fCurrentBytesPerRow; };

protected:
			addr_t			fBase;
			phys_addr_t		fPhysicalBase;
			size_t			fSize;
			int				fCurrentWidth;
			int				fCurrentHeight;
			int				fCurrentDepth;
			int				fCurrentBytesPerRow;
};


#endif /* _ARCH_FRAMEBUFFER_H */
