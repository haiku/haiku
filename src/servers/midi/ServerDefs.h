/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 */

#ifndef MIDI_SERVER_DEFS_H
#define MIDI_SERVER_DEFS_H

#include <List.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>

// Describes an application that registered with the midi_server.
struct app_t
{
	// For sending notifications to the app.
	BMessenger messenger;
};

// Describes a MIDI endpoint. The endpoint_t structure is
// used to describe both consumer and producer endpoints.
struct endpoint_t
{
#ifdef DEBUG
	endpoint_t()
	{
		app = (app_t*) 0xbaadc0de;
	}
#endif

	// The application that owns this endpoint.
	app_t* app;

	// The endpoint's system-wide ID, which is assigned 
	// by the midi_server when the endpoint is created.
	int32 id;

	// Is this a consumer or producer endpoint?
	bool consumer;

	// Whether this endpoint is visible to all applications.
	bool registered;

	// The endpoint's human-readable name.
	BString name;

	// User-defined attributes.
	BMessage properties;

	// The port that accepts MIDI events (consumer only).
	port_id port;

	// How long it takes this endpoint to process incoming
	// MIDI events (consumer only). 
	bigtime_t latency;

	// Which consumers this endpoint sprays outgoing MIDI 
	// events to (producer only). 
	BList connections;
};

#endif // MIDI_SERVER_DEFS_H
