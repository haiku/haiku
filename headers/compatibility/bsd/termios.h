/*
 * Copyright 2023 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_TERMIOS_H_
#define _BSD_TERMIOS_H_


#include_next <termios.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

extern int cfsetspeed(struct termios *termios, speed_t speed);

#ifdef __cplusplus
}
#endif


#endif


#endif  /* _BSD_TERMIOS_H_ */
