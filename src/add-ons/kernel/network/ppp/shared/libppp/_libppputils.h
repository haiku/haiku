/*
 * Copyright 2003-2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef __libppputils__h
#define __libppputils__h

#include <net_stack_driver.h>
#include <NetServer.h>


#define PPP_SERVER_SIGNATURE	NET_SERVER_SIGNATURE


char *get_stack_driver_path();


#endif
