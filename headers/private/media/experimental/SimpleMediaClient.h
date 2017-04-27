/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MEDIA_SIMPLE_CLIENT_H
#define _MEDIA_SIMPLE_CLIENT_H

#include <MediaClient.h>
#include <MediaConnection.h>


namespace BPrivate { namespace media {


class BSimpleMediaInput;
class BSimpleMediaOutput;

class BSimpleMediaClient : public BMediaClient {
public:
	enum notification {
		B_WILL_START = 1,			// performance_time
		B_WILL_STOP,				// performance_time immediate
		B_WILL_SEEK,				// performance_time media_time

		B_FORMAT_SUGGESTION,		// media_type type, int32 quality,
									// media_format* format
	};

	typedef void					(*notify_hook)(void* cookie,
											notification what,
											...);

									BSimpleMediaClient(const char* name,
										media_type type
											= B_MEDIA_UNKNOWN_TYPE,
										media_client_kinds
											kinds = B_MEDIA_PLAYER
												& B_MEDIA_RECORDER);

	virtual							~BSimpleMediaClient();

	// This is supplied to support generic connections not related
	// to a certain destination or source node, however for various ambiguity
	// reasons we want the BMediaConnection to be declared as input or output.
	// You can pass the object returned by this function to another
	// BMediaClient::Connect(), so that it will automatically connect to this node.
	virtual BSimpleMediaInput*		BeginInput();
	virtual BSimpleMediaOutput*		BeginOutput();

			void					SetHook(notify_hook notifyHook = NULL,
										void* cookie = NULL);

protected:
	virtual void					HandleStart(bigtime_t performanceTime);
	virtual void					HandleStop(bigtime_t performanceTime);

	virtual void					HandleSeek(bigtime_t mediaTime,
										bigtime_t performanceTime);

	virtual status_t				FormatSuggestion(media_type type,
										int32 quality, media_format* format);

private:
			notify_hook				fNotifyHook;
			void*					fNotifyCookie;

	virtual	void					_ReservedSimpleMediaClient0();
	virtual	void					_ReservedSimpleMediaClient1();
	virtual	void					_ReservedSimpleMediaClient2();
	virtual	void					_ReservedSimpleMediaClient3();
	virtual	void					_ReservedSimpleMediaClient4();
	virtual	void					_ReservedSimpleMediaClient5();
			uint32					fPadding[32];
};


class BSimpleMediaConnection : public virtual BMediaConnection {
public:
	enum notification {
		// Inputs
		B_INPUT_CONNECTED = 1,
		B_INPUT_DISCONNECTED,

		B_FORMAT_CHANGED,

		// Outputs
		B_OUTPUT_CONNECTED,
		B_OUTPUT_DISCONNECTED,

		B_PREPARE_TO_CONNECT,	// media_format* format, media_source* source,
								// char* name

		B_FORMAT_PROPOSAL,		// media_format* format
		B_ASK_FORMAT_CHANGE,
	};

	// This function is called when it is the moment to handle a buffer.
	typedef void					(*process_hook)(
										BMediaConnection* connection,
										BBuffer* buffer);

	// Used to notify or inquire the client about what to do when certain
	// events happen.
	typedef status_t				(*notify_hook)(
										BMediaConnection* connection,
										notification what,
										...);

			// Use this to set your callbacks.
			void					SetHooks(process_hook processHook = NULL,
										notify_hook notifyHook = NULL,
										void* cookie = NULL);

			void*					Cookie() const;

protected:
									BSimpleMediaConnection(
										media_connection_kinds kinds);
	virtual							~BSimpleMediaConnection();

			process_hook			fProcessHook;
			notify_hook				fNotifyHook;
			void*					fBufferCookie;
};


class BSimpleMediaInput : public BSimpleMediaConnection, public BMediaInput {
public:
									BSimpleMediaInput();

protected:
	virtual							~BSimpleMediaInput();

	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

	virtual void					HandleBuffer(BBuffer* buffer);
};


class BSimpleMediaOutput : public BSimpleMediaConnection, public BMediaOutput {
public:
									BSimpleMediaOutput();

protected:
	virtual							~BSimpleMediaOutput();

	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

	virtual status_t				FormatProposal(media_format* format);
};


}

}

using namespace BPrivate::media;

#endif
