/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <Archivable.h>
#include <String.h>


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

#endif
