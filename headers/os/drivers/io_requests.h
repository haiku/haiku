/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _IO_REQUESTS_H
#define _IO_REQUESTS_H

/*! I/O request interface */


#include <SupportDefs.h>


typedef struct IORequest io_request;


#ifdef __cplusplus
extern "C" {
#endif

bool io_request_is_write(const io_request* request);

#ifdef __cplusplus
}
#endif

#endif	/* _IO_REQUESTS_H */
