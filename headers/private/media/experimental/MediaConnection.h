/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MEDIA_CONNECTION_H
#define _MEDIA_CONNECTION_H

#include <BufferGroup.h>
#include <MediaDefs.h>

#include <MediaClient.h>
#include <MediaClientDefs.h>


namespace BPrivate { namespace media {

class BMediaClientNode;


// The BMediaConnection class is the swiss knife of BMediaClient.
// It represents a connection between two nodes and allow to create complex
// nodes without dealing with the unneeded complexity. Two local connections,
// can be binded, this means that when you will receive a buffer A as input,
// the BufferReceived function will be called so that you can process the BBuffer,
// and once the function returns the output will be automatically forwarded
// to the connection B SendBuffer method.
// It's not possible to mix a BMediaInput with a BMediaOutput in the same class.
class BMediaConnection {
public:
	const media_connection&			Connection() const;
	BMediaClient*					Client() const;

	media_connection_id				Id() const;
	const char*						Name() const;

	bool							HasBinding() const;
	BMediaConnection*				Binding() const;

	bool							IsConnected() const;

	// This allow to specify a format that will be used while
	// connecting to another node.
	void							SetAcceptedFormat(
										const media_format& format);
	const media_format&				AcceptedFormat() const;

	// Represents the buffer size, depends on the format set or negotiated
	// for this connection.
	size_t							BufferSize() const;

	// Disconnect this connection. When a connection is disconnected,
	// it can be reused as brand new.
	status_t						Disconnect();

	// Once you are done with this connection you release it, it automatically
	// remove the object from the BMediaClient and free all used resources.
	// This will make the connection to disappear completely, so if you
	// want to preserve it for future connections just Disconnect() it.
	status_t						Release();

protected:
									BMediaConnection(
										media_connection_kinds kinds);
	virtual							~BMediaConnection();

	// Those callbacks are shared between BMediaInput and BMediaOutput
	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

	// Specify a latency range to allow the connection behave correctly.
	// Ideally the minimum latency should be the algorithmic latency you expect
	// from the node and will be used as starting point. The max latency is the
	// maximum acceptable by you, over that point the node will adjust it's
	// performance time to recover if a big delay happen.
			void					SetLatencyRange(bigtime_t min,
										bigtime_t max);

			void					GetLatencyRange(bigtime_t* min,
										bigtime_t* max) const;

private:

			void					_ConnectionRegistered(BMediaClient* owner,
										media_connection_id id);

	const media_source&				_Source() const;
	const media_destination&		_Destination() const;

	media_connection				fConnection;

	BMediaClient*					fOwner;

	// A connection might be binded so that it will automatically
	// forward or receive the data from/to a local BMediaConnection,
	// see BMediaClient::Bind.
	BMediaConnection*				fBind;

	BBufferGroup*					fBufferGroup;

	bool							fConnected;

	bigtime_t						fMinLatency;
	bigtime_t						fMaxLatency;

	virtual	void					_ReservedMediaConnection0();
	virtual	void					_ReservedMediaConnection1();
	virtual	void					_ReservedMediaConnection2();
	virtual	void					_ReservedMediaConnection3();
	virtual	void					_ReservedMediaConnection4();
	virtual	void					_ReservedMediaConnection5();
	virtual	void					_ReservedMediaConnection6();
	virtual	void					_ReservedMediaConnection7();
	virtual	void					_ReservedMediaConnection8();
	virtual	void					_ReservedMediaConnection9();
	virtual	void					_ReservedMediaConnection10();
	uint32							fPadding[64];

	friend class BMediaClient;
	friend class BMediaClientNode;

	friend class BMediaInput;
	friend class BMediaOutput;
};


class BMediaInput : public virtual BMediaConnection {
public:
									BMediaInput();

protected:
	virtual							~BMediaInput();

	// Callbacks
	virtual status_t				FormatChanged(const media_format& format);

	virtual void					HandleBuffer(BBuffer* buffer);

private:
	media_input						_MediaInput() const;

	virtual	void					_ReservedMediaInput0();
	virtual	void					_ReservedMediaInput1();
	virtual	void					_ReservedMediaInput2();
	virtual	void					_ReservedMediaInput3();
	virtual	void					_ReservedMediaInput4();
	virtual	void					_ReservedMediaInput5();
	virtual	void					_ReservedMediaInput6();
	virtual	void					_ReservedMediaInput7();
	virtual	void					_ReservedMediaInput8();
	virtual	void					_ReservedMediaInput9();
	virtual	void					_ReservedMediaInput10();
	uint32							fPadding[32];

	friend class BMediaClientNode;
};


class BMediaOutput : public virtual BMediaConnection {
public:
									BMediaOutput();

	void							SetEnabled(bool enabled);
	bool							IsEnabled() const;

protected:
	virtual							~BMediaOutput();

	// Callbacks
	virtual status_t				PrepareToConnect(media_format* format);

	virtual status_t				FormatProposal(media_format* format);
	virtual status_t				FormatChangeRequested(media_format* format);

	// When a connection is not binded with another, and you really don't want
	// to use BMediaGraph it's your job to send the buffer to the connection
	// you want. You might want to ovverride it so that you can track something,
	// in this case be sure to call the base version. Be sure to know what
	// you are doing.
	virtual	status_t				SendBuffer(BBuffer* buffer);

private:
	media_output					_MediaOutput() const;

	bool							fEnabled;
	size_t							fFramesSent;

	virtual	void					_ReservedMediaOutput0();
	virtual	void					_ReservedMediaOutput1();
	virtual	void					_ReservedMediaOutput2();
	virtual	void					_ReservedMediaOutput3();
	virtual	void					_ReservedMediaOutput4();
	virtual	void					_ReservedMediaOutput5();
	virtual	void					_ReservedMediaOutput6();
	virtual	void					_ReservedMediaOutput7();
	virtual	void					_ReservedMediaOutput8();
	virtual	void					_ReservedMediaOutput9();
	virtual	void					_ReservedMediaOutput10();
	uint32							fPadding[32];

	friend class BMediaClientNode;
};


}

}

using namespace BPrivate::media;

#endif
