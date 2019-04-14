/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef OUTPUT_STREAM_H
#define OUTPUT_STREAM_H

#include "Transport.h"


class OutputStream
{
public:
	virtual void	Write(const void *buffer, size_t size)
						/* throw(TransportException) */ = 0;
};


#endif
