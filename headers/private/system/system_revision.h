/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYSTEM_REVISION_H
#define _SYSTEM_SYSTEM_REVISION_H


/** The length of the system revision character array symbol living in libroot
    and the kernel */
#define SYSTEM_REVISION_LENGTH 128


#ifdef __cplusplus
extern "C" {
#endif


/** returns the system revision */
const char* get_system_revision();


#ifdef __cplusplus
}
#endif


#endif	/* _SYSTEM_SYSTEM_REVISION_H */
