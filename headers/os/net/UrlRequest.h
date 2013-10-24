/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <OS.h>


class BUrlRequest {
public:
									BUrlRequest(const BUrl& url,
										BUrlProtocolListener* listener,
										BUrlContext* context,
										const char* threadName,
										const char* protocolName);
	virtual							~BUrlRequest();

	// URL protocol thread management
	virtual	thread_id				Run();
	virtual status_t				Pause();
	virtual status_t				Resume();
	virtual	status_t				Stop();

	// URL protocol parameters modification
			status_t				SetUrl(const BUrl& url);
			status_t				SetContext(BUrlContext* context);
			status_t				SetListener(BUrlProtocolListener* listener);

	// URL protocol parameters access
			const BUrl&				Url() const;
			BUrlContext*			Context() const;
			BUrlProtocolListener*	Listener() const;
			const BString&			Protocol() const;

	// URL protocol informations
			bool					IsRunning() const;
			status_t				Status() const;
	virtual const char*				StatusString(status_t threadStatus)
										const;


protected:
	static	int32					_ThreadEntry(void* arg);
	virtual	status_t				_ProtocolLoop();
	virtual void					_EmitDebug(BUrlProtocolDebugMessage type,
										const char* format, ...);
protected:
			BUrl					fUrl;
			BUrlContext*			fContext;
			BUrlProtocolListener*	fListener;

			bool					fQuit;
			bool					fRunning;
			status_t				fThreadStatus;
			thread_id				fThreadId;
			BString					fThreadName;
			BString					fProtocol;
};


// TODO: Rename, this is in the global namespace.
enum {
	B_PROT_THREAD_STATUS__BASE	= 0,
	B_PROT_SUCCESS = B_PROT_THREAD_STATUS__BASE,
	B_PROT_RUNNING,
	B_PROT_PAUSED,
	B_PROT_ABORTED,
	B_PROT_SOCKET_ERROR,
	B_PROT_CONNECTION_FAILED,
	B_PROT_CANT_RESOLVE_HOSTNAME,
	B_PROT_WRITE_FAILED,
	B_PROT_READ_FAILED,
	B_PROT_NO_MEMORY,
	B_PROT_PROTOCOL_ERROR,
		//  Thread status over this one are guaranteed to be
		// errors
	B_PROT_THREAD_STATUS__END
};

#endif // _B_URL_REQUEST_H_
