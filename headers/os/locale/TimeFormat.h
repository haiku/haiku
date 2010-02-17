/* 
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_TIME_FORMAT_H_
#define _B_TIME_FORMAT_H_


#include <NumberFormat.h>
#include <SupportDefs.h>


class BString;


class BTimeFormat : public BNumberFormat {
	public:
		status_t Format(int64 number, BString *buffer) const;

		// TODO : version for char* buffer, size_t bufferSize
		// TODO : parsing ?	
};


#endif
