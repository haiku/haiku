/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPMediaIO.h"

#include <Handler.h>
#include <HttpRequest.h>
#include <UrlProtocolRoster.h>

#include "MediaDebug.h"


// 10 seconds timeout
#define HTTP_TIMEOUT 10000000

using namespace BPrivate::Network;


class FileListener : public BUrlProtocolListener, public BDataIO {
public:
		FileListener(HTTPMediaIO* owner)
			:
			BUrlProtocolListener(),
			fRequest(NULL),
			fAdapterIO(owner),
			fInitSem(-1),
			fRunning(false)
		{
			fInputAdapter = fAdapterIO->BuildInputAdapter();
			fInitSem = create_sem(0, "http_streamer init sem");
		}

		virtual ~FileListener()
		{
			_ReleaseInit();
		}

		bool ConnectionSuccessful() const
		{
			return fRequest != NULL;
		}

		void ConnectionOpened(BUrlRequest* request)
		{
			fRequest = request;
			fRunning = true;
		}

		void HeadersReceived(BUrlRequest* request)
		{
			if (request != fRequest) {
				delete request;
				return;
			}

			fAdapterIO->UpdateSize();
		}

		void RequestCompleted(BUrlRequest* request, bool success)
		{
			_ReleaseInit();

			if (request != fRequest)
				return;

			fRequest = NULL;
		}

		ssize_t Write(const void* data, size_t size)
		{
			_ReleaseInit();

			return fInputAdapter->Write(data, size);
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
	fReq(NULL),
	fListener(NULL),
	fReqThread(-1),
	fUrl(url),
	fIsMutable(false)
{
	CALLED();
}


HTTPMediaIO::~HTTPMediaIO()
{
	CALLED();

	if (fReq != NULL) {
		fReq->Stop();
		status_t status;
		wait_for_thread(fReqThread, &status);

		delete fReq;
	}
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
	CALLED();

	fListener = new FileListener(this);

	fReq = BUrlProtocolRoster::MakeRequest(fUrl, fListener, fListener);

	if (fReq == NULL)
		return B_ERROR;

	BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(fReq);
	if (httpReq != NULL)
		httpReq->SetStopOnError(true);

	fReqThread = fReq->Run();
	if (fReqThread < 0)
		return B_ERROR;

	status_t ret = fListener->LockOnInit(HTTP_TIMEOUT);
	if (ret != B_OK)
		return ret;

	return BAdapterIO::Open();
}


bool
HTTPMediaIO::IsRunning() const
{
	if (fListener != NULL)
		return fListener->IsRunning();
	else if (fReq != NULL)
		return fReq->IsRunning();

	return false;
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
