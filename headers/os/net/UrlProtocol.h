/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_H_
#define _B_URL_PROTOCOL_H_


#include <Url.h>
#include <UrlResult.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <OS.h>


class BUrlProtocol {
public:
									BUrlProtocol(const BUrl& url,
										BUrlProtocolListener* listener,
										BUrlContext* context,
										BUrlResult* result,
										const char* threadName,
										const char* protocolName);
	virtual							~BUrlProtocol();

	// URL protocol required members
	// TODO: (stippi) I know it's sometimes appealing to have these
	// "simplistic" methods that can do anything, but they remove
	// type-safety. Why not overload SetOption with all possible types?
	// Like:
	//		SetOption(uint32 option, bool value);
	//		SetOption(uint32 option, int8 value);
	//		SetOption(uint32 option, int16 value);
	//		SetOption(uint32 option, int32 value);
	//		...
	// This keeps the calling side even more simple, since it
	// don't need to do pointer stuff with the parameters. Also, since
	// this method is forced to be implemented in derived classes, why
	// is it here at all? Why not have non-virtual setters for specific
	// things, where the setter is properly named?
	virtual	status_t				SetOption(uint32 name, void* value) = 0;

	// URL protocol thread management
	virtual	thread_id				Run();
	virtual status_t				Pause();
	virtual status_t				Resume();
	virtual	status_t				Stop();

	// URL protocol parameters modification
			status_t				SetUrl(const BUrl& url);
			status_t				SetResult(BUrlResult* result);
			status_t				SetContext(BUrlContext* context);
			status_t				SetListener(BUrlProtocolListener* listener);

	// URL protocol parameters access
			const BUrl&				Url() const;
			BUrlResult*				Result() const;
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

	// URL result parameters access
			BMallocIO&				_ResultRawData();
			BHttpHeaders&			_ResultHeaders();
			void					_SetResultStatusCode(int32 statusCode);
			BString&				_ResultStatusText();

protected:
			BUrl					fUrl;
			BUrlResult*				fResult;
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


namespace BPrivate {

class BUrlProtocolOption {
public:
				BUrlProtocolOption(void* value) : fValuePtr(value) { }

		bool	Bool() const	{ return *reinterpret_cast<bool*>(fValuePtr); }
		int8 	Int8() const	{ return *reinterpret_cast<int8*>(fValuePtr); }
		int16 	Int16() const	{ return *reinterpret_cast<int16*>(fValuePtr); }
		int32 	Int32() const	{ return *reinterpret_cast<int32*>(fValuePtr); }
		char*	String() const	{ return reinterpret_cast<char*>(fValuePtr); }
		void*	Pointer() const	{ return fValuePtr; }

private:
		void*	fValuePtr;
};

} // namespace BPrivate


#endif // _B_URL_PROTOCOL_H_
