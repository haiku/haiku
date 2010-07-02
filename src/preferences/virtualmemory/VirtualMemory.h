/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H


#include <Application.h>
#include <Catalog.h>
#include <Locale.h>

class VMSettings;


class VirtualMemory : public BApplication {
	public:
		VirtualMemory();
		virtual ~VirtualMemory();

		virtual void ReadyToRun();
		virtual void AboutRequested();

	private:
		void GetCurrentSettings(bool& enabled, off_t& size);

		VMSettings *fSettings;	
};
	
#endif	/* VIRTUAL_MEMORY_H */
