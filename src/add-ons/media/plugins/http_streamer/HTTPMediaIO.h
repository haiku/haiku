/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_MEDIA_IO_H
#define _HTTP_MEDIA_IO_H


#include <DataIO.h>
#include <HttpRequest.h>
#include <Url.h>
#include <UrlContext.h>


class HTTPMediaIO : public BMediaIO {
public:
							HTTPMediaIO(BUrl* url);
	virtual					~HTTPMediaIO();

			status_t		InitCheck() const;

	virtual	ssize_t			ReadAt(off_t position, void* buffer,
								size_t size);
	virtual	ssize_t			WriteAt(off_t position, const void* buffer,
								size_t size);
	virtual	off_t			Seek(off_t position, uint32 seekMode);
	virtual off_t			Position() const;

	virtual	status_t		SetSize(off_t size);
	virtual	status_t		GetSize(off_t* size) const;

	virtual bool			IsSeekable() const;
	virtual	bool			IsEndless() const;

private:
	status_t				_IntegrityCheck();

	BUrlContext*			fContext;
	BHttpRequest*			fReq;

	BMallocIO*				fBuffer;
	status_t				fInitErr;
};

#endif
