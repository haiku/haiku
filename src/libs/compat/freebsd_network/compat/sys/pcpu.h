/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_PCPU_H_
#define _FBSD_COMPAT_SYS_PCPU_H_


#include <thread.h>


#define curthread (thread_get_current_thread())

#endif /* _FBSD_COMPAT_SYS_PCPU_H_ */
