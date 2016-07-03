/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPMediaIO.h"

#include <Handler.h>
#include <UrlProtocolRoster.h>


// 10 seconds timeout
#define HTTP_TIMEOUT 10000000


class FileListener : public BUrlProtocolAsynchronousListener {
public:
		FileListener(BAdapterIO* owner)
			:
			BUrlProtocolAsynchronousListener(true),
			fRequest(NULL),
			fAdapterIO(owner),
			fInitSem(-1)
		{
			fInputAdapter = fAdapterIO->BuildInputAdapter();
			fInitSem = create_sem(0, "http_streamer init sem");
		}

		virtual ~FileListener() {};

		bool ConnectionSuccessful() const
		{
			return fRequest != NULL;
		}

		void ConnectionOpened(BUrlRequest* request)
		{
			if (fRequest != NULL)
				fRequest->Stop();

			fRequest = request;
		}

		void DataReceived(BUrlRequest* request, const char* data,
			off_t position, ssize_t size)
		{
			if (fInitSem != -1) {
				release_sem(fInitSem);
				delete_sem(fInitSem);
				fInitSem = -1;
			}

			if (request != fRequest)
				delete request;

			fInputAdapter->Write(data, size);
		}

		void RequestCompleted(BUrlRequest* request, bool success)
		{
			if (request != fRequest)
				return;

			fRequest = NULL;
			delete request;
		}

		status_t LockOnInit(bigtime_t timeout)
		{
			return acquire_sem_etc(fInitSem, 1, B_RELATIVE_TIMEOUT, timeout);
		}

private:
		BUrlRequest*	fRequest;
		BAdapterIO*		fAdapterIO;
		BInputAdapter*	fInputAdapter;
		sem_id			fInitSem;
};


HTTPMediaIO::HTTPMediaIO(BUrl url)
	:
	BAdapterIO(HTTP_TIMEOUT),
	fContext(NULL),
	fReq(NULL),
	fListener(NULL),
	fUrl(url),
	fIsMutable(false)
{
}


HTTPMediaIO::~HTTPMediaIO()
{
}


void
HTTPMediaIO::GetFlags(int32* flags) const
{
	*flags = B_MEDIA_STREAMING | B_MEDIA_SEEK_BACKWARD;
	if (fIsMutable)
		*flags = *flags | B_MEDIA_MUTABLE_SIZE;
}


ssize_t
HTTPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
HTTPMediaIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
HTTPMediaIO::Open()
{
	fContext = new BUrlContext();
	fContext->AcquireReference();

	fListener = new FileListener(this);

	fReq = BUrlProtocolRoster::MakeRequest(fUrl,
		fListener, fContext);

	if (fReq == NULL)
		return B_ERROR;

	if (fReq->Run() < 0)
		return B_ERROR;

	status_t ret = fListener->LockOnInit(HTTP_TIMEOUT);
	if (ret != B_OK)
		return ret;

	off_t totalSize = fReq->Result().Length();

	// At this point we decide if our size is fixed or mutable,
	// this will change the behavior of our parent.
	if (totalSize > 0)
		BAdapterIO::SetSize(totalSize);
	else
		fIsMutable = true;

	return BAdapterIO::Open();
}


void
HTTPMediaIO::Close()
{
	delete fReq;
	delete fListener;

	fContext->ReleaseReference();
	delete fContext;

	BAdapterIO::Close();
}


status_t
HTTPMediaIO::SeekRequested(off_t position)
{
	return BAdapterIO::SeekRequested(position);
}
