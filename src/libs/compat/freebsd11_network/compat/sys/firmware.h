/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_FIRMWARE_H_
#define _FBSD_COMPAT_SYS_FIRMWARE_H_


#define FIRMWARE_UNLOAD 0x0001


struct firmware {
	const char*		name;		// system-wide name
	const void*		data;		// location of image
	size_t			datasize;	// size of image in bytes
	unsigned int	version;	// version of the image
};


const struct firmware* firmware_get(const char*);
void firmware_put(const struct firmware*, int);

#endif /* _FBSD_COMPAT_SYS_FIRMWARE_H_ */
