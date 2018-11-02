/*
 * Copyright 2011-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _ARCH_MAILBOX_H
#define _ARCH_MAILBOX_H


#include <boot/platform.h>
#include <SupportDefs.h>


#define TRACE_MAILBOX
#ifdef TRACE_MAILBOX
#   define TRACE(x...) dprintf(x)
#	define CALLED() dprintf("%s()\n", __func__);
#else
#   define TRACE(x...) ;
#	define CALLED() ;
#endif
#define ERROR(x...) dprintf(x)


class ArchMailbox {
public:
							ArchMailbox(addr_t base)
								:
								fBase(base) {};
							~ArchMailbox() {};

	virtual status_t		Init() { return B_OK; };
	virtual status_t		Probe() { return B_OK; };

	virtual addr_t			Base() { return fBase; };
			addr_t			PhysicalBase() { return fPhysicalBase; };

	virtual	status_t		Write(uint8 channel, uint32 value) { return B_OK; };
	virtual status_t		Read(uint8 channel, uint32& value) { return B_OK; };


protected:
			addr_t			fBase;
			addr_t			fPhysicalBase;
};


#endif /* _ARCH_MAILBOX_H */
