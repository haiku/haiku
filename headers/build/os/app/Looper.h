/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOOPER_H
#define _LOOPER_H


#include <OS.h>


// Port (Message Queue) Capacity
#define B_LOOPER_PORT_DEFAULT_CAPACITY  200


class BLooper {
public:
								BLooper(const char* name = NULL,
                                	int32 priority = B_NORMAL_PRIORITY,
	                                int32 port_capacity
    	                                = B_LOOPER_PORT_DEFAULT_CAPACITY);

			bool				Lock()		{ return true; }
			void				Unlock()	{}
};


#endif	// _LOOPER_H
