/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SHARED_H_
#define SHARED_H_


#define MAX_DEVICES	8


struct ifnet;

struct device {
	struct device	*parent;
	struct device	*root;

	driver_t		*driver;
	struct list		children;

	uint32			flags;

	char			device_name[128];
	int				unit;
	char			nameunit[64];
	const char		*description;
	void			*softc;
	void			*ivars;

	struct {
		int (*probe)(device_t dev);
		int (*attach)(device_t dev);
		int (*detach)(device_t dev);
		int (*suspend)(device_t dev);
		int (*resume)(device_t dev);
		void (*shutdown)(device_t dev);

		int (*miibus_readreg)(device_t, int, int);
		int (*miibus_writereg)(device_t, int, int, int);
		void (*miibus_statchg)(device_t);
		void (*miibus_linkchg)(device_t);
		void (*miibus_mediainit)(device_t);
	} methods;

	struct list_link link;
};


extern const char *gDeviceNameList[];
extern struct ifnet *gDevices[];
extern int32 gDeviceCount;

#endif /* SHARED_H_ */
