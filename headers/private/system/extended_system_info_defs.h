/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_EXTENDED_SYSTEM_INFO_DEFS_H
#define _SYSTEM_EXTENDED_SYSTEM_INFO_DEFS_H


enum {
	B_TEAM_INFO_BASIC				= 0x01,	// basic general info
	B_TEAM_INFO_THREADS				= 0x02,	// list of threads
	B_TEAM_INFO_IMAGES				= 0x04,	// list of images
	B_TEAM_INFO_AREAS				= 0x08,	// list of areas
	B_TEAM_INFO_SEMAPHORES			= 0x10,	// list of semaphores
	B_TEAM_INFO_PORTS				= 0x20,	// list of ports
	B_TEAM_INFO_FILE_DESCRIPTORS	= 0x40	// list of file descriptors
};


#endif	/* _SYSTEM_EXTENDED_SYSTEM_INFO_DEFS_H */
