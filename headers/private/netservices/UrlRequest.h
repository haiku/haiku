/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlResult.h>
#include <OS.h>
#include <Referenceable.h>


namespace BPrivate {

namespace Network {


class BUrlRequest {
public:

									BUrlRequest(const BUrl& url,
										BDataIO* output,
										BUrlProtocolListener* listener,
										BUrlContext* context,
										const char* threadName,
										const char* protocolName);
	virtual							~BUrlRequest();

	// URL protocol thread management
	virtual	thread_id				Run();

	virtual	status_t				Stop();
	virtual void					SetTimeout(bigtime_t timeout) {}

	// URL protocol parameters modification
			status_t				SetUrl(const BUrl& url);
			status_t				SetContext(BUrlContext* context);
			status_t				SetListener(BUrlProtocolListener* listener);
			status_t				SetOutput(BDataIO* output);

	// URL protocol parameters access
			const BUrl&				Url() const;
			BUrlContext*			Context() const;
			BUrlProtocolListener*	Listener() const;
			const BString&			Protocol() const;
			BDataIO*				Output() const;

	// URL protocol informations
			bool					IsRunning() const;
			status_t				Status() const;
	virtual const BUrlResult&		Result() const = 0;


protected:
	static	int32					_ThreadEntry(void* arg);
	virtual	void					_ProtocolSetup() {};
	virtual	status_t				_ProtocolLoop() = 0;
	virtual void					_EmitDebug(BUrlProtocolDebugMessage type,
										const char* format, ...);
protected:
			BUrl					fUrl;
			BReference<BUrlContext>	fContext;
			BUrlProtocolListener*	fListener;
			BDataIO*				fOutput;

			bool					fQuit;
			bool					fRunning;
			status_t				fThreadStatus;
			thread_id				fThreadId;
			BString					fThreadName;
			BString					fProtocol;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_REQUEST_H_
