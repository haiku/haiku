/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <new>

#include <UrlRequest.h>
#include <UrlProtocolHttp.h>
#include <Debug.h>


BUrlRequest::BUrlRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context)
	:
	fListener(listener),
	fUrlProtocol(NULL),
	fResult(url),
	fContext(context),
	fUrl(),
	fInitStatus(B_ERROR)
{
	SetUrl(url);
}


BUrlRequest::BUrlRequest(const BUrlRequest& other)
	:
	fListener(NULL),
	fUrlProtocol(NULL),
	fResult(other.fUrl),
	fContext(),
	fUrl(),
	fInitStatus(B_ERROR)
{
	*this = other;
}


BUrlRequest::~BUrlRequest()
{
	if (fUrlProtocol != NULL)
		delete fUrlProtocol;
}

// #pragma mark Request parameters modification


status_t
BUrlRequest::SetUrl(const BUrl& url)
{
	fUrl = url;
	fResult.SetUrl(url);

	if (fUrlProtocol != NULL && url.Protocol() == fUrl.Protocol()) {
		fUrlProtocol->SetUrl(url);
		return B_OK;
	}

	fInitStatus = _SetupProtocol();
	return fInitStatus;
}


void
BUrlRequest::SetContext(BUrlContext* context)
{
	fContext = context;
}


void
BUrlRequest::SetProtocolListener(BUrlProtocolListener* listener)
{
	fListener = listener;

	if (fUrlProtocol != NULL)
		fUrlProtocol->SetListener(listener);
}


bool
BUrlRequest::SetProtocolOption(int32 option, void* value)
{
	if (fUrlProtocol == NULL)
		return false;

	fUrlProtocol->SetOption(option, value);
	return true;
}


// #pragma mark Request parameters access


const BUrlProtocol*
BUrlRequest::Protocol()
{
	return fUrlProtocol;
}


const BUrlResult&
BUrlRequest::Result()
{
	return fResult;
}


const BUrl&
BUrlRequest::Url()
{
	return fUrl;
}


// #pragma mark Request control


status_t
BUrlRequest::Start()
{
	if (fUrlProtocol == NULL) {
		PRINT(("BUrlRequest::Perform() : Oops, no BUrlProtocol defined!\n"));
		return B_ERROR;
	}

	thread_id protocolThread = fUrlProtocol->Run();

	if (protocolThread < B_OK)
		return protocolThread;

	return B_OK;
}


status_t
BUrlRequest::Pause()
{
	if (fUrlProtocol == NULL)
		return B_ERROR;

	return fUrlProtocol->Pause();
}


status_t
BUrlRequest::Resume()
{
	if (fUrlProtocol == NULL)
		return B_ERROR;

	return fUrlProtocol->Resume();
}


status_t
BUrlRequest::Abort()
{
	if (fUrlProtocol == NULL)
		return B_ERROR;

	status_t returnCode = fUrlProtocol->Stop();
	delete fUrlProtocol;
	fUrlProtocol = NULL;

	return returnCode;
}


// #pragma mark Request informations


status_t
BUrlRequest::InitCheck() const
{
	return fInitStatus;
}


bool
BUrlRequest::IsRunning() const
{
	if (fUrlProtocol == NULL)
		return false;

	return fUrlProtocol->IsRunning();
}


status_t
BUrlRequest::Status() const
{
	if (fUrlProtocol == NULL)
		return B_ERROR;

	return fUrlProtocol->Status();
}


// #pragma mark Overloaded members


BUrlRequest&
BUrlRequest::operator=(const BUrlRequest& other)
{
	delete fUrlProtocol;
	fUrlProtocol = NULL;
	fUrl = other.fUrl;
	fListener = other.fListener;
	fContext = other.fContext;
	fResult = BUrlResult(other.fUrl);
	SetUrl(other.fUrl);

	return *this;
}


status_t
BUrlRequest::_SetupProtocol()
{
	// TODO: instanciate the correct BUrlProtocol w/ the services roster
	delete fUrlProtocol;
	fUrlProtocol = NULL;

	if (fUrl.Protocol() == "http")
		fUrlProtocol = new(std::nothrow) BUrlProtocolHttp(fUrl, false, "HTTP",
			fListener, fContext, &fResult);
	else if (fUrl.Protocol() == "https")
		fUrlProtocol = new(std::nothrow) BUrlProtocolHttp(fUrl, true, "HTTPS",
			fListener, fContext, &fResult);
	else
		return B_NO_HANDLER_FOR_PROTOCOL;

	if (fUrlProtocol == NULL)
		return B_NO_MEMORY;

	return B_OK;
}
