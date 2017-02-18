/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MEDIA_CLIENT_H
#define _MEDIA_CLIENT_H

#include <ObjectList.h>
#include <Buffer.h>

#include <MediaAddOn.h>
#include <MediaClientDefs.h>
#include <MediaDefs.h>
#include <MediaNode.h>


namespace BPrivate { namespace media {


class BMediaClientNode;
class BMediaConnection;
class BMediaInput;
class BMediaOutput;

// Private stuff
class InputReleaser;
class OutputReleaser;


// BMediaClient is a general purpose class allowing to create any kind
// of media_node. It automatically manage the expected behavior under
// different run modes, and allow to specify the different capabilities needed.
// BMediaClient is not using any of the coding patterns you might be used to.
// There are no events to care, and threading is managed internally using
// the data processing specified by the BMediaGraph class.
class BMediaClient {
public:
									BMediaClient(const char* name,
										media_type type
											= B_MEDIA_UNKNOWN_TYPE,
										media_client_kinds
											kind = B_MEDIA_PLAYER
												& B_MEDIA_RECORDER);

	virtual							~BMediaClient();

			const media_client&		Client() const;

			media_client_id			Id() const;
			const char*				Name() const;
	// Return the capabilities of this BMediaClient instance.
			media_client_kinds		Kinds() const;
			media_type				MediaType() const;

			status_t				InitCheck() const;

	// TODO: Should allow BControllable capabilities
	// TODO: Add file interface
	// TODO: Offline mode is still missing

	// When those functions return, the BMediaConnection is added to the
	// list and is visible to other nodes as not connected. Any input/output
	// should be registered to a BMediaClient to become visible in the system.
	virtual status_t				RegisterInput(BMediaInput* input);
	virtual status_t				RegisterOutput(BMediaOutput* output);

	// Bind internally two connections of the same BMediaClient, so that the
	// input will be automatically forwarded to the output just after the
	// ProcessFunc is called. The buffer is automatically recycled too.
	// Beware that the binding operation is valid only for local connections
	// which belong to this node, otherwise return B_ERROR.
	virtual status_t				Bind(BMediaInput* input,
										BMediaOutput* output);

	virtual status_t				Unbind(BMediaInput* input,
										BMediaOutput* output);

	// If the user want a particular format for a connection it should
	// use BMediaConnection::SetAcceptedFormat(), if it's not specified
	// BMediaClient::Format() will be used, in case both aren't specified
	// an error is returned. The first parameter should always belong to
	// this node, the second will be a connection obtained from another
	// BMediaClient. Unregistered connections will be registered automatically.
	virtual status_t				Connect(BMediaConnection* ourConnection,
										BMediaConnection* theirConnection);

	virtual status_t				Connect(BMediaConnection* ourConnection,
										const media_connection& theirConnection);

	// Find a free input/output and try to connect to the media_client,
	// return meaningful error otherwise.
	virtual status_t				Connect(BMediaConnection* ourConnection,
										const media_client& client);

	// Disconnect any connection belonging to this object, to disconnect
	// a single connection use BMediaConnection::Disconnect().
	virtual status_t				Disconnect();

			int32					CountInputs() const;
			int32					CountOutputs() const;

			BMediaInput*			InputAt(int32 index) const;
			BMediaOutput*			OutputAt(int32 index) const;

			BMediaInput*			FindInput(
										const media_connection& input) const;
			BMediaOutput*			FindOutput(
										const media_connection& output) const;

			bool					IsRunning() const;

	// NOTE: The following functions aren't provided to be inherited,
	// always use the protected HandleSomething version. This is because
	// otherwise you could break the connection mechanism and mine interoperability
	// from remote nodes.
			status_t				Start();
			status_t				Stop();
			status_t				Seek(bigtime_t mediaTime,
										bigtime_t performanceTime);
			status_t				Roll(bigtime_t start, bigtime_t stop,
										bigtime_t seek);

	// Preroll the client to buffer startup latency
			status_t				Preroll();

	// This function return when the client reach the specified performanceTime
			status_t				SyncTo(bigtime_t performanceTime,
										bigtime_t timeout = -1);

	// It will be B_INCREASE_LATENCY by default
			BMediaNode::run_mode	RunMode() const;
			status_t				SetRunMode(BMediaNode::run_mode mode);

	// Return the current performance time handled by the object when
	// run_mode != B_OFFLINE. Otherwise returns the current offline time.
			bigtime_t				CurrentTime() const;

	// This is supplied to support using this class in a BMediaAddOn.
	// Default version just return NULL.
	virtual	BMediaAddOn*			AddOn(int32* id) const;

protected:
	virtual void					HandleStart(bigtime_t performanceTime);
	virtual void					HandleStop(bigtime_t performanceTime);

	virtual void					HandleSeek(bigtime_t mediaTime,
										bigtime_t performanceTime);

	virtual status_t				FormatSuggestion(media_type type,
										int32 quality, media_format* format);

private:
			void					_Init();
			void					_Deinit();

			void					_AddInput(BMediaInput* input);
			void					_AddOutput(BMediaOutput* output);

			BMediaInput*			_FindInput(
										const media_destination& dest) const;
			BMediaOutput*			_FindOutput(
										const media_source& source) const;

			status_t				_ConnectInput(BMediaOutput* output,
										const media_connection& input);
			status_t				_ConnectOutput(BMediaInput* input,
										const media_connection& output);

			status_t				_DisconnectConnection(BMediaConnection* conn);
			status_t				_ReleaseConnection(BMediaConnection* conn);

			status_t				fInitErr;

			media_client			fClient;

			bool					fRunning;
			BMediaClientNode*		fNode;

			bigtime_t				fCurrentTime;

			BObjectList<InputReleaser>	fInputs;
			BObjectList<OutputReleaser>	fOutputs;

			media_connection_id		fLastID;

	virtual	void					_ReservedMediaClient0();
	virtual	void					_ReservedMediaClient1();
	virtual	void					_ReservedMediaClient2();
	virtual	void					_ReservedMediaClient3();
	virtual	void					_ReservedMediaClient4();
	virtual	void					_ReservedMediaClient5();
	virtual	void					_ReservedMediaClient6();
	virtual	void					_ReservedMediaClient7();
	virtual	void					_ReservedMediaClient8();
	virtual	void					_ReservedMediaClient9();
	virtual	void					_ReservedMediaClient10();
			uint32					fPadding[64];

	friend class BMediaClientNode;
	friend class BMediaConnection;
	friend class BMediaInput;
	friend class BMediaOutput;
};


}

}

using namespace BPrivate::media;

#endif
