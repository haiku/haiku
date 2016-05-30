/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_MEDIA_IO_H
#define _HTTP_MEDIA_IO_H


#include <AdapterIO.h>
#include <FileRequest.h>
#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolAsynchronousListener.h>


class FileListener;

class HTTPMediaIO : public BAdapterIO {
public:
										HTTPMediaIO(BUrl* url);
	virtual								~HTTPMediaIO();

			status_t					InitCheck() const;

	virtual	ssize_t						WriteAt(off_t position,
											const void* buffer, size_t size);

private:
	BUrlContext*						fContext;
	BUrlRequest*						fReq;
	FileListener*						fListener;

	status_t							fInitErr;
};

#endif
