/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Ma Jie, china.majie at gmail
 */
#ifndef POOR_MAN_LOG_H
#define POOR_MAN_LOG_H

#include <netinet/in.h>

#include "constants.h" //for rgb_color BLACK

#ifdef __cplusplus
	extern "C"
	void poorman_log(
		const char* msg,
		bool needTimeHeader = true,
		in_addr_t addr = INADDR_NONE,
		rgb_color color = BLACK
	);
#else //c version is for libhttpd
	void poorman_log(
		const char* msg,
		bool needTimeHeader,
		in_addr_t addr,
		rgb_color color
	);
#endif


#endif	// POOR_MAN_LOG_H
