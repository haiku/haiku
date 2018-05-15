/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_SIGNAL_H
#define _KERNEL_COMPAT_SIGNAL_H


struct compat_sigaction {
	union {
		uint32		sa_handler;
		uint32		sa_sigaction;
	};
	sigset_t				sa_mask;
	int						sa_flags;
	uint32					sa_userdata;	/* will be passed to the signal
											   handler, BeOS extension */
} _PACKED;


#endif // _KERNEL_COMPAT_SIGNAL_H
