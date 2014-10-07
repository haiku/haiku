/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_MESSAGE_FORMAT_H_
#define _B_MESSAGE_FORMAT_H_


#include <Format.h>


class BMessageFormat: public BFormat {
public:
			status_t			Format(BString& buffer, const BString message,
									const int32 arg);
};


#endif
