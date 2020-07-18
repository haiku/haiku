/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <String.h>


#ifdef LIBNETAPI_DEPRECATED
#include <Archivable.h>


class BUrlResult: public BArchivable {
public:
							BUrlResult();
							BUrlResult(BMessage*);
	virtual					~BUrlResult();

	virtual	status_t		Archive(BMessage*, bool) const;

			void			SetContentType(BString contentType);
			void			SetLength(size_t length);

	virtual	BString			ContentType() const;
	virtual size_t			Length() const;

	static	BArchivable*	Instantiate(BMessage*);

private:
			BString			fContentType;
			size_t			fLength;
};

#else

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

}

}

#endif // LIBNETAPI_DEPRECATED

#endif // _B_URL_RESULT_H_
