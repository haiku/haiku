/*
 * Copyright 2009-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BOSII_DRIVER_H_
#define _BOSII_DRIVER_H_


#include <ether_driver.h>


#define ETHER_OP_CODES_END (ETHER_GET_LINK_STATE + 1)

// TODO: those will be removed again
/* ioctl() opcodes a wlan driver should support */
enum {
	BOSII_DEVICE = ETHER_OP_CODES_END,
	BOSII_DETECT_NETWORKS,
	BOSII_GET_DETECTED_NETWORKS,
	BOSII_JOIN_NETWORK,
	BOSII_GET_ASSOCIATED_NETWORK
};

#endif /* _BOSII_DRIVER_H_ */
