/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <String.h>


namespace BPrivate {

namespace Network {


class BUrlResult {
public:
							BUrlResult();
	virtual					~BUrlResult();

			void			SetContentType(BString contentType);
			void			SetLength(off_t length);

	virtual	BString			ContentType() const;
	virtual	off_t			Length() const;

private:
			BString			fContentType;
			off_t			fLength;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_RESULT_H_
