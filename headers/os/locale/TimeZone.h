/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_ZONE_H
#define _TIME_ZONE_H


#include <String.h>


class BTimeZone {
public:
								BTimeZone(const char* zoneCode = NULL);
								~BTimeZone();

			const BString&		Code() const;
			const BString&		Name() const;
			int					OffsetFromGMT() const;

			status_t			InitCheck() const;

private:
			void				_Init(const char* zoneCode);

			BString				fCode;
			BString				fName;
			int					fOffsetFromGMT;

			status_t			fInitStatus;
};


#endif	// _TIME_ZONE_H
