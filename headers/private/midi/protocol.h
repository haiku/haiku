/*
 * Definitions for the communications protocol between libmidi2.so
 * and the midi_server.
 *
 * Copyright (c) 2002-2003 Matthijs Hollemans
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
#ifndef MIDI_PROTOCOL_H
#define MIDI_PROTOCOL_H

// MIME signature of the midi_server application.
#define MIDI_SERVER_SIGNATURE  "application/x-vnd.Haiku-midi_server"

// Timeout for delivering and responding to messages (microseconds).
#define TIMEOUT  2000000

// Received when a new app starts using the Midi Kit.
#define MSG_REGISTER_APP  'Mapp'

// Sent when we have completed a "register app" request.
#define MSG_APP_REGISTERED  'mAPP'

// Received when an app creates a new local endpoint.
#define MSG_CREATE_ENDPOINT  'Mnew'

// Sent to all other applications when an app creates a 
// new endpoint. Also sent when an application registers 
// with the midi_server (MSG_REGISTER_APP). 
#define MSG_ENDPOINT_CREATED  'mNEW'

// Received when an app deletes a local endpoint.
#define MSG_DELETE_ENDPOINT  'Mdel'

// The midi_server sends this message to itself when an app 
// dies and its endpoints must be removed from the roster.
#define MSG_PURGE_ENDPOINT  'Mdie'

// Sent to all applications when an endpoint is deleted,
// either by the app that owned it, or by the midi_server
// if the owner app has died.
#define MSG_ENDPOINT_DELETED  'mDEL'

// Received when an app changes the attributes of one 
// of its local endpoints. 
#define MSG_CHANGE_ENDPOINT  'Mchg'

// Sent to all other applications when an app changes
// the attributes of one of its local endpoints.
#define MSG_ENDPOINT_CHANGED  'mCHG'

// Received when an app wants to establish a connection
// between a producer and a consumer.
#define MSG_CONNECT_ENDPOINTS  'Mcon'

// Sent to all other applications when an app establishes
// a connection between a producer and a consumer. Like
// MSG_ENDPOINT_CREATED, this notification is also sent to
// applications when they register with the midi_server.
#define MSG_ENDPOINTS_CONNECTED 'mCON'

// Received when an app wants to break a connection
// between a producer and a consumer.
#define MSG_DISCONNECT_ENDPOINTS  'Mdis'

// Sent to all other applications when an app breaks
// a connection between a producer and a consumer.
#define MSG_ENDPOINTS_DISCONNECTED 'mDIS'

#endif // MIDI_PROTOCOL_H
