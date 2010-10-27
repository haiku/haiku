/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_TIME_H_
#define _B_HTTP_TIME_H_

#include <ctime>

#include <String.h>

namespace BPrivate {

enum {
	B_HTTP_TIME_FORMAT_PARSED = -1,
	B_HTTP_TIME_FORMAT_RFC1123 = 0,
	B_HTTP_TIME_FORMAT_PREFERRED = B_HTTP_TIME_FORMAT_RFC1123,
	B_HTTP_TIME_FORMAT_COOKIE,
	B_HTTP_TIME_FORMAT_RFC1036,
	B_HTTP_TIME_FORMAT_ASCTIME
};


class BHttpTime {
public:
						BHttpTime();
						BHttpTime(time_t date);
						BHttpTime(const BString& dateString);
	
	// Date modification
			void		SetString(const BString& string);
			void		SetDate(time_t date);
	
		
	// Date conversion
			time_t		Parse();
			BString		ToString(int8 format = B_HTTP_TIME_FORMAT_PARSED);
			
private:
			BString		fDateString;
			time_t 		fDate;
			int8		fDateFormat;
};

}
#endif // _B_HTTP_TIME_H_
