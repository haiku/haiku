/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef OF_SUPPORT_H
#define OF_SUPPORT_H


#include <OS.h>


bigtime_t system_time(void);
uint32 of_address_cells(int package);
uint32 of_size_cells(int package);

#endif
