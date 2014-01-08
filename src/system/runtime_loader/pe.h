/*
 * Copyright 2013-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *  Alexander von Gluck IV, <kallisti5@unixzen.com>
 */
#ifndef PE_H
#define PE_H


#include <OS.h>

#include <syscalls.h>
#include <util/kernel_cpp.h>

#include "pe_common.h"


status_t pe_verify_header(void *header, size_t length);


#endif /* PE_H */
