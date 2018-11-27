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

using BCodecKit::BAdapterIO;
using BCodecKit::BInputAdapter;


class FileListener;

class HTTPMediaIO : public BAdapterIO {
public:
										HTTPMediaIO(BUrl url);
	virtual								~HTTPMediaIO();

	virtual void						GetFlags(int32* flags) const;

	virtual	ssize_t						WriteAt(off_t position,
											const void* buffer, size_t size);

	virtual status_t					SetSize(off_t size);

	virtual status_t					Open();

	virtual bool						IsRunning() const;

protected:
	virtual	status_t					SeekRequested(off_t position);

	// Other custom stuff

			void						UpdateSize();

	friend class FileListener;
private:
	BUrlRequest*						fReq;
	FileListener*						fListener;
	thread_id							fReqThread;

	BUrl								fUrl;
	off_t								fTotalSize;
	bool								fIsMutable;
};

#endif
