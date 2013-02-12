/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <HttpHeaders.h>
#include <Url.h>
#include <UrlResult.h>
#include <UrlContext.h>
#include <UrlProtocol.h>
#include <UrlProtocolListener.h>


enum {
	B_NO_HANDLER_FOR_PROTOCOL = B_ERROR
};


class BUrlRequest {
public:
									BUrlRequest(const BUrl& url,
										BUrlProtocolListener* listener = NULL,
										BUrlContext* context = NULL);
									BUrlRequest(const BUrlRequest& other);
	virtual							~BUrlRequest();

	// Request parameters modification
			status_t				SetUrl(const BUrl& url);
			void					SetContext(BUrlContext* context);
			void					SetProtocolListener(
										BUrlProtocolListener* listener);
			bool					SetProtocolOption(int32 option,
										void* value);
	// Request parameters access
			const BUrlProtocol*		Protocol();
			const BUrlResult&		Result();
			const BUrl&				Url();

	// Request control
	virtual	status_t				Start();
	virtual status_t				Pause();
	virtual status_t				Resume();
	virtual status_t				Abort();

	// Request informations
	virtual	status_t				InitCheck() const;
			bool					IsRunning() const;
			status_t				Status() const;

	// Overloaded members
			BUrlRequest&			operator=(const BUrlRequest& other);


protected:
			BUrlProtocolListener*	fListener;
			BUrlProtocol*			fUrlProtocol;
			BUrlResult				fResult;
			BUrlContext*			fContext;
			BUrl					fUrl;
			status_t				fInitStatus;

private:
			status_t				_SetupProtocol();
};

#endif // _B_URL_REQUEST_H_
