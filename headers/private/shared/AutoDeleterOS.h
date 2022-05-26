/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_OS_H
#define _AUTO_DELETER_OS_H


#include <AutoDeleter.h>
#include <OS.h>


namespace BPrivate {


typedef HandleDeleter<area_id, status_t, delete_area> AreaDeleter;
typedef HandleDeleter<sem_id, status_t, delete_sem> SemDeleter;
typedef HandleDeleter<port_id, status_t, delete_port> PortDeleter;


}


using ::BPrivate::AreaDeleter;
using ::BPrivate::SemDeleter;
using ::BPrivate::PortDeleter;


#endif	// _AUTO_DELETER_OS_H
