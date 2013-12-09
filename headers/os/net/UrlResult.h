/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <String.h>


class BUrlResult {
public:
	virtual					~BUrlResult();
			void			SetContentType(BString contentType);
			void			SetLength(size_t length);

	virtual	BString			ContentType() const;
	virtual size_t			Length() const;

private:
			BString			fContentType;
			BString			fCharset;
			size_t			fLength;
};

#endif
