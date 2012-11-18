/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H


#include <Application.h>


class VirtualMemory : public BApplication {
public:
					VirtualMemory();
	virtual			~VirtualMemory();

	virtual	void	ReadyToRun();
	virtual	void	AboutRequested();
};

#endif	/* VIRTUAL_MEMORY_H */
