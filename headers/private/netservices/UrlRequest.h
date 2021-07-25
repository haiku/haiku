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


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

class BUrlRequest {
public:
#ifdef LIBNETAPI_DEPRECATED
									BUrlRequest(const BUrl& url,
										BUrlProtocolListener* listener,
										BUrlContext* context,
										const char* threadName,
										const char* protocolName);
#else
									BUrlRequest(const BUrl& url,
										BDataIO* output,
										BUrlProtocolListener* listener,
										BUrlContext* context,
										const char* threadName,
										const char* protocolName);
#endif
	virtual							~BUrlRequest();

	// URL protocol thread management
	virtual	thread_id				Run();
	virtual	status_t				Stop();
	virtual void					SetTimeout(bigtime_t timeout) {}

	// URL protocol parameters modification
			status_t				SetUrl(const BUrl& url);
			status_t				SetContext(BUrlContext* context);
			status_t				SetListener(BUrlProtocolListener* listener);
#ifndef LIBNETAPI_DEPRECATED
			status_t				SetOutput(BDataIO* output);
#endif

	// URL protocol parameters access
			const BUrl&				Url() const;
			BUrlContext*			Context() const;
			BUrlProtocolListener*	Listener() const;
			const BString&			Protocol() const;
#ifndef LIBNETAPI_DEPRECATED
			BDataIO*				Output() const;
#endif

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
#ifndef LIBNETAPI_DEPRECATED
			BDataIO*				fOutput;
#endif

			bool					fQuit;
			bool					fRunning;
			status_t				fThreadStatus;
			thread_id				fThreadId;
			BString					fThreadName;
			BString					fProtocol;
};

#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif // _B_URL_REQUEST_H_
