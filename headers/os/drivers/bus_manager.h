/*******************************************************************************
/
/	File:			bus_managers.h
/
/	Description:	bus manager API
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _BUS_MANAGER_H
#define _BUS_MANAGER_H

#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bus_manager_info bus_manager_info;

struct bus_manager_info {
	module_info		minfo;
	status_t		(*rescan)();
};

#ifdef __cplusplus
}
#endif

#endif	/* _BUS_MANAGER_H */
