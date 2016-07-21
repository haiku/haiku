/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPMediaIO.h"

#include <Handler.h>
#include <HttpRequest.h>
#include <UrlProtocolRoster.h>


// 10 seconds timeout
#define HTTP_TIMEOUT 10000000


class FileListener : public BUrlProtocolAsynchronousListener {
public:
		FileListener(HTTPMediaIO* owner)
			:
			BUrlProtocolAsynchronousListener(true),
			fRequest(NULL),
			fAdapterIO(owner),
			fInitSem(-1),
			fRunning(false)
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
			fAdapterIO->UpdateSize();

			fRequest = request;
			fRunning = true;
		}

		void DataReceived(BUrlRequest* request, const char* data,
			off_t position, ssize_t size)
		{
			if (request != fRequest)
				delete request;

			BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(request);
			const BHttpResult& httpRes = (const BHttpResult&)httpReq->Result();
			if (httpReq != NULL) {
				int32 status = httpRes.StatusCode();
				if (BHttpRequest::IsClientErrorStatusCode(status)
						|| BHttpRequest::IsServerErrorStatusCode(status)) {
					fRunning = false;
				} else if (BHttpRequest::IsRedirectionStatusCode(status))
					return;
			}

			_ReleaseInit();

			fInputAdapter->Write(data, size);
		}

		void RequestCompleted(BUrlRequest* request, bool success)
		{
			_ReleaseInit();

			if (request != fRequest)
				return;

			fRequest = NULL;
			delete request;
		}

		status_t LockOnInit(bigtime_t timeout)
		{
			return acquire_sem_etc(fInitSem, 1, B_RELATIVE_TIMEOUT, timeout);
		}

		bool IsRunning()
		{
			return fRunning;
		}

private:
		void _ReleaseInit()
		{
			if (fInitSem != -1) {
				release_sem(fInitSem);
				delete_sem(fInitSem);
				fInitSem = -1;
			}
		}

		BUrlRequest*	fRequest;
		HTTPMediaIO*	fAdapterIO;
		BInputAdapter*	fInputAdapter;
		sem_id			fInitSem;
		bool			fRunning;
};


HTTPMediaIO::HTTPMediaIO(BUrl url)
	:
	BAdapterIO(B_MEDIA_STREAMING | B_MEDIA_SEEKABLE, HTTP_TIMEOUT),
	fContext(NULL),
	fReq(NULL),
	fReqThread(-1),
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

	fReqThread = fReq->Run();
	if (fReqThread < 0)
		return B_ERROR;

	status_t ret = fListener->LockOnInit(HTTP_TIMEOUT);
	if (ret != B_OK)
		return ret;

	return BAdapterIO::Open();
}


void
HTTPMediaIO::Close()
{
	fReq->Stop();
	status_t status;
	wait_for_thread(fReqThread, &status);

	delete fReq;
	delete fListener;

	fContext->ReleaseReference();
	delete fContext;

	BAdapterIO::Close();
}


bool
HTTPMediaIO::IsRunning() const
{
	return fListener->IsRunning();
}


status_t
HTTPMediaIO::SeekRequested(off_t position)
{
	return BAdapterIO::SeekRequested(position);
}


void
HTTPMediaIO::UpdateSize()
{
	// At this point we decide if our size is fixed or mutable,
	// this will change the behavior of our parent.
	off_t size = fReq->Result().Length();
	if (size > 0)
		BAdapterIO::SetSize(size);
	else
		fIsMutable = true;
}
