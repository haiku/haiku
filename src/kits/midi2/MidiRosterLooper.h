/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef MIDI_ROSTER_LOOPER_H
#define MIDI_ROSTER_LOOPER_H

#include <Looper.h>
#include <Message.h>

class BMidiRoster;

namespace BPrivate {

// Receives messages from the midi_server on behalf of the 
// BMidiRoster. Also keeps track of the list of endpoints.
class BMidiRosterLooper : public BLooper
{
public:

	BMidiRosterLooper();
	virtual ~BMidiRosterLooper();

	// Starts up the looper.
	bool Init(BMidiRoster* roster);

	// Does the work for BMidiRoster::NextEndpoint().
	BMidiEndpoint* NextEndpoint(int32* id);

	// Finds an endpoint in our list of endpoints.
	BMidiEndpoint* FindEndpoint(int32 id);

	// Adds an endpoint to our list.
	void AddEndpoint(BMidiEndpoint* endp);

	// Removes an endpoint from our list. Throws away
	// any connections that this endpoint is part of. 
	void RemoveEndpoint(BMidiEndpoint* endp);

	// Invoked when the client app wants to be kept informed
	// about changes in the roster. From now on, we will send
 	// the messenger B_MIDI_EVENT notifications when something
	// interesting happens. But first, we send a whole bunch
	// of notifications about the registered remote endpoints, 
	// and the connections between them.
	void StartWatching(const BMessenger* watcher);

	// From now on, we will no longer send notifications to
	// the client when something interesting happens.
	void StopWatching();

	virtual void MessageReceived(BMessage* msg);
	
private:

	friend class BMidiRoster;
	friend class BMidiProducer;

	typedef BLooper super;

	void OnAppRegistered(BMessage* msg);
	void OnEndpointCreated(BMessage* msg);
	void OnEndpointDeleted(BMessage* msg);
	void OnEndpointChanged(BMessage* msg);
	void OnConnectedDisconnected(BMessage* msg);

	void ChangeRegistered(BMessage* msg, BMidiEndpoint* endp);
	void ChangeName(BMessage* msg, BMidiEndpoint* endp);
	void ChangeProperties(BMessage* msg, BMidiEndpoint* endp);
	void ChangeLatency(BMessage* msg, BMidiEndpoint* endp);

	// Removes the consumer from the list of connections of
	// all the producers it is connected to. Also sends out 
	// B_MIDI_EVENT "disconnected" notifications if the 
	// consumer is remote and the client is watching.
	void DisconnectDeadConsumer(BMidiConsumer* cons);

	// Sends out B_MIDI_EVENT "disconnected" notifications
	// if the producer is remote and the client is watching.
	void DisconnectDeadProducer(BMidiProducer* prod);

	// Sends B_MIDI_EVENT notifications for all registered 
	// remote endpoints to the watcher. Used when the client
	// calls StartWatching().
	void AllEndpoints();

	// Sends B_MIDI_EVENT notifications for the connections
	// between all registered remote endpoints to the watcher.
	// Used when the client calls StartWatching().
	void AllConnections();

	// Sends a B_MIDI_EVENT notification to the watcher
	// when another application changes the attributes 
	// of one of its endpoints.
	void ChangeEvent(BMessage* msg, BMidiEndpoint* endp);

	// Sends a B_MIDI_EVENT notification to the watcher
	// when another application connects or disconnects
	// the two endpoints.
	void ConnectionEvent(
		BMidiProducer* prod, BMidiConsumer* cons, bool mustConnect);

	int32 CountEndpoints();
	BMidiEndpoint* EndpointAt(int32 index);

	BMidiRoster* roster;

	// Makes sure BMidiRoster::MidiRoster() does not return 
	// until confirmation from the midi_server is received.
	sem_id initLock;

	// The object we send B_MIDI_EVENT notifications to.
	BMessenger* watcher;

	// All the endpoints in the system, local and remote.
	BList endpoints;

	#ifdef DEBUG
	void DumpEndpoints();
	#endif
};

} // namespace BPrivate

#endif // MIDI_ROSTER_LOOPER_H
