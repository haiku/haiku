/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include "device.h"

#include <stdlib.h>

#include <compat/sys/bus.h>
#include <compat/sys/mbuf.h>


static status_t
compat_open(const char *name, uint32 flags, void **cookie)
{
	return B_ERROR;
}


static status_t
compat_close(void *cookie)
{
	device_t dev = cookie;
	return B_ERROR;
}


static status_t
compat_free(void *cookie)
{
	device_t dev = cookie;
	free(dev);
	return B_ERROR;
}


static status_t
compat_read(void *cookie, off_t position, void *buf, size_t *numBytes)
{
	uint32 semFlags = B_CAN_INTERRUPT;
	device_t dev = cookie;
	status_t status;
	struct mbuf *mb;
	size_t len;

	if (atomic_or(&dev->flags, 0) & DEVICE_CLOSED)
		return B_INTERRUPTED;

	if (atomic_or(&dev->flags, 0) & DEVICE_NON_BLOCK)
		semFlags |= B_RELATIVE_TIMEOUT;


	do {
		status = acquire_sem_etc(dev->receive_sem, 1, semFlags, 0);
		if (atomic_or(&dev->flags, 0) & DEVICE_CLOSED)
			return B_INTERRUPTED;

		if (status == B_WOULD_BLOCK) {
			*numBytes = 0;
			return B_OK;
		} else if (status < B_OK)
			return status;

		IF_DEQUEUE(&dev->receive_queue, mb);
	} while (mb == NULL);

	len = min_c(max_c(mb->m_len, 0), *numBytes);

	// TODO we need something that cna join various chunks
	memcpy(buf, mtod(mb, const void *), len);
	*numBytes = len;

	m_freem(mb);
	return B_OK;
}


static status_t
compat_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	return B_ERROR;
}


static status_t
compat_control(void *cookie, uint32 op, void *arg, size_t len)
{
	return B_ERROR;
}
