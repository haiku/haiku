/*
 * Copyright 2015-2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MEDIA_CLIENT_DEFS_H
#define _MEDIA_CLIENT_DEFS_H

#include <MediaDefs.h>
#include <MediaNode.h>


namespace BPrivate { namespace media {


typedef int64 media_client_id;
typedef int64 media_connection_id;

typedef uint64 media_client_kinds;
typedef uint64 media_connection_kinds;


enum media_client_kind {
	// The client can receive media data.
	B_MEDIA_RECORDER		= 0x000000001,
	// The client can send media data to another client.
	B_MEDIA_PLAYER			= 0x000000002,
	// The client specify a control GUI which can be used to configure it.
	B_MEDIA_CONTROLLABLE	= 0x000000004
};

enum media_connection_kind {
	B_MEDIA_INPUT = 0,
	B_MEDIA_OUTPUT = 1
};


typedef struct media_client {
	media_client_id				Id() const;
	media_client_kinds			Kinds() const;

	BMessage*					ToMessage();

private:
	media_client_kinds			kinds;

	media_node					node;
	uint32						padding[16];

	friend class BMediaClient;
	friend class BMediaConnection;
	friend struct media_connection;
} media_client;


typedef struct media_connection {
	media_connection_id			Id() const;
	media_connection_kinds		Kinds() const;

	const media_client&			Client() const;

	const char*					Name() const;

	bool						IsInput() const;
	bool						IsOutput() const;

	BMessage*					ToMessage() const;

private:
	// NOTE: We are doing this on purpose to avoid redundancy we
	// want to build the input/output on the fly. In pratice, the
	// only thing that can change is the format which is kept updated
	// to reflect the current status of this connection.
	media_input					_BuildMediaInput() const;
	media_output				_BuildMediaOutput() const;

	media_node					_Node() const;

	media_connection_id			id;

	media_client				client;

	media_node					remote_node;

    media_source				source;
    media_destination			destination;

    // This format always reflect the most updated format depending
    // on the connection phase.
    media_format				format;
    char						name[B_MEDIA_NAME_LENGTH];

	media_connection_kinds		kinds;
	uint32						padding[16];

	friend class BMediaClient;
	friend class BMediaClientNode;
	friend class BMediaConnection;
	friend class BMediaInput;
	friend class BMediaOutput;
} media_connection;


}

}

using namespace BPrivate::media;

#endif
