/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// InfoWindowManager.h
//
// * PURPOSE
//	 Manages all the ParameterWindows and control panels.
//   Will not let you open multiple windows referring to
//	 the same node, and takes care of quitting them all
//	 when shut down.
//
// * HISTORY
//   c.lenz		17feb2000		Begun
//

#ifndef __InfoWindowManager_H__
#define __InfoWindowManager_H__

#include <Looper.h>
#include <Point.h>

class BList;
class BWindow;

#include "cortex_defs.h"

struct dormant_node_info;
struct media_destination;
struct media_input;
struct media_output;
struct media_source;

__BEGIN_CORTEX_NAMESPACE

class Connection;
class NodeRef;

class InfoWindowManager :
	public	BLooper {

public:								// *** constants

	// the screen position where the first window should
	// be displayed
	static const BPoint				M_INIT_POSITION;

	// horizontal/vertical offset by which subsequent
	// windows positions are shifted
	static const BPoint 			M_DEFAULT_OFFSET;

	enum message_t {
									M_INFO_WINDOW_REQUESTED = InfoView_message_base,

									M_LIVE_NODE_WINDOW_CLOSED,

									M_DORMANT_NODE_WINDOW_CLOSED,

									M_CONNECTION_WINDOW_CLOSED,

									M_INPUT_WINDOW_CLOSED,

									M_OUTPUT_WINDOW_CLOSED
	};

private:							// *** ctor/dtor

	// hidden ctor; is called only from inside Instance()
									InfoWindowManager();

public:

	// quits all registered info windows
	virtual							~InfoWindowManager();

public:								// *** singleton access

	// access to the one and only instance of this class
	static InfoWindowManager	   *Instance();

	// will delete the singleton instance and take down all
	// open windows
	static void						shutDown();

public:								// *** operations

	status_t						openWindowFor(
										const NodeRef *ref);

	status_t						openWindowFor(
										const dormant_node_info &info);

	status_t						openWindowFor(
										const Connection &connection);

	status_t						openWindowFor(
										const media_input &input);

	status_t						openWindowFor(
										const media_output &output);

public:								// *** BLooper impl

	virtual void					MessageReceived(
										BMessage *message);

private:							// *** internal operations

	// management of windows for live nodes
	bool							_addWindowFor(
										const NodeRef *ref,
										BWindow *window);
	bool							_findWindowFor(
										int32 nodeID,
										BWindow **outWindow);
	void							_removeWindowFor(
										int32 nodeID);

	// management of windows for dormant nodes
	bool							_addWindowFor(
										const dormant_node_info &info,
										BWindow *window);
	bool							_findWindowFor(
										const dormant_node_info &info,
										BWindow **outWindow);
	void							_removeWindowFor(
										const dormant_node_info &info);

	// management of windows for connections
	bool							_addWindowFor(
										const Connection &connection,
										BWindow *window);
	bool							_findWindowFor(
										const media_source &source,
										const media_destination &destination,
										BWindow **outWindow);
	void							_removeWindowFor(
										const media_source &source,
										const media_destination &destination);

	// management of windows for media_inputs
	bool							_addWindowFor(
										const media_input &input,
										BWindow *window);
	bool							_findWindowFor(
										const media_destination &destination,
										BWindow **outWindow);
	void							_removeWindowFor(
										const media_destination &destination);

	// management of windows for media_outputs
	bool							_addWindowFor(
										const media_output &output,
										BWindow *window);
	bool							_findWindowFor(
										const media_source &source,
										BWindow **outWindow);
	void							_removeWindowFor(
										const media_source &source);

private:							// *** data members

	// list of all currently open windows about live nodes
	BList						   *m_liveNodeWindows;

	// list of all currently open windows about dormant nodes
	BList						   *m_dormantNodeWindows;

	// list of all currently open windows about connections
	BList						   *m_connectionWindows;

	// list of all currently open windows about media_inputs
	BList						   *m_inputWindows;

	// list of all currently open windows about media_outputs
	BList						   *m_outputWindows;

	// the BPoint at which the last InfoWindow was initially
	// opened
	BPoint							m_nextWindowPosition;

private:							// *** static members

	// the magic singleton instance
	static InfoWindowManager  *s_instance;
};

__END_CORTEX_NAMESPACE
#endif /*__InfoWindowManager_H__*/
