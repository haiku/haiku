/*
 * Copyright 2015-2018, Dario Casalinuovo. All rights reserved.
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

	// If the connection is connected get the other endpoint,
	// return media_connection::null otherwise.
	media_connection				Endpoint();

	// Represents the buffer size, implement it to return the buffer size
	// you decided for this connection.
	// TODO: Do we want this (and ChainSize) moved on the output side?
	// Or perhaps provide an implementation based on the buffer group
	// for the consumer?
	// Problem is: the consumer has not easy access to the buffer group,
	// so we way want to add a special messaging between clients after
	// connection, so that inputs know the buffer size and chain size.
	virtual size_t					BufferSize() const = 0;
	// Implement it to specify the size of your chain of buffers.
	//virtual int32					ChainSize() const = 0;

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
										media_connection_kinds kinds,
										const char* name = NULL);
	virtual							~BMediaConnection();

private:
	// Those callbacks are shared between BMediaInput and BMediaOutput
	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

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

	bool							fConnected;

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
protected:
									BMediaInput(const char* name = NULL);
	virtual							~BMediaInput();

	// Callbacks
	virtual status_t				AcceptFormat(media_format* format) = 0;

	virtual void					HandleBuffer(BBuffer* buffer);

	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

private:

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
protected:
									BMediaOutput(const char* name = NULL);
	virtual							~BMediaOutput();

	// Callbacks
	virtual status_t				PrepareToConnect(media_format* format) = 0;
	virtual status_t				FormatProposal(media_format* format) = 0;

	// When a connection is not binded with another, and you really don't want
	// to use BMediaGraph it's your job to send the buffer to the connection
	// you want. You might want to ovverride it so that you can track something,
	// in this case be sure to call the base version. Be sure to know what
	// you are doing.
	virtual	status_t				SendBuffer(BBuffer* buffer);

	virtual void					Connected(const media_format& format);
	virtual void					Disconnected();

private:

	// TODO: possibly unneeded.
	void							_SetEnabled(bool enabled);
	bool							_IsEnabled() const;

	bool							fEnabled;
	size_t							fFramesSent;

	BBufferGroup*					fBufferGroup;

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
