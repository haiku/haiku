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


// Connection.h (Cortex)
// * PURPOSE
//   Represents a general connection between two media nodes.
//
// * NOTES 13aug99
//   Connection is undergoing massive resimplification(TM).
//   1) It's now intended to be stored and passed by value; synchronization
//      and reference issues are no more.
//   2) It now refers to the participatory nodes by ID, not pointer.  This
//      makes the nodes slightly more cumbersome to look up, but makes
//      Connection completely 'safe': an outdated instance doesn't contain
//      any dangerous information.
//
// * NOTES 29jul99
//   1) For completeness, a release() or disconnect() method would be nice.  Problems?
//   2) How will connections between 'external' nodes be denoted?  For example,
//      the audioMixer->audioOutput connection must not be broken EVER.  Other external
//      connections might be manually broken, but should be left alone when the
//      NodeManager quits (whereas all internal connections need to be removed by
//      the dtor.)  This implies two flags: 'internal' and 'locked'...
//
// * HISTORY
//   e.moon		25jun99		Begun

#ifndef __Connection_H__
#define __Connection_H__

#include <Debug.h>
#include <MediaNode.h>
#include <String.h>

#include "cortex_defs.h"

__BEGIN_CORTEX_NAMESPACE

class NodeManager;
class NodeGroup;
class NodeRef;

class Connection {

	// rather incestuous set of classes we've got here...
	friend class NodeRef;
	friend class NodeManager;

public:					// *** types & constants
	enum flag_t {
		// connection should be removed automatically when the NodeManager
		//   is destroyed
		INTERNAL					= 1<<1,
		// connection must never be removed
		LOCKED						= 1<<2
	};

public:					// *** dtor/user-level ctors
	virtual ~Connection();
	
	Connection();
	Connection(
		const Connection&					clone); //nyi
	Connection& operator=(
		const Connection&					clone); //nyi

public:					// *** accessors

	uint32 id() const																{ return m_id; }

	bool isValid() const														{ return
																										m_sourceNode != media_node::null &&
																										m_destinationNode != media_node::null &&
																										!m_disconnected; }

	// changed 13aug99
	media_node_id sourceNode() const 								{ return m_sourceNode.node; }
	const media_source& source() const 							{ return m_source; }
	const char* outputName() const 									{ return m_outputName.String(); }
	
	// changed 13aug99
	media_node_id destinationNode() const 					{ return m_destinationNode.node; }
	const media_destination& destination() const 		{ return m_destination; }
	const char* inputName() const 									{ return m_inputName.String(); }
	
	const media_format& format() const 							{ return m_format; }
	
	uint32 flags() const														{ return m_flags; }

	// input/output access [e.moon 14oct99]
	status_t getInput(
		media_input*							outInput) const;

	status_t getOutput(
		media_output*							outOutput) const;

	// hint access

	status_t getOutputHint(
		const char**							outName,
		media_format*							outFormat) const;

	status_t getInputHint(
		const char**							outName,
		media_format*							outFormat) const;
		
	const media_format& requestedFormat() const { return m_requestedFormat; }


protected:				// *** general ctor accessible to subclasses &
									//     cortex::NodeManager

	// id must be non-0
	Connection(
		uint32										id,
		media_node								srcNode,
		const media_source&				src,
		const char*								outputName,
		media_node								destNode,
		const media_destination&	dest,
		const char*								inputName,
		const media_format&				format,
		uint32										flags);

	// if any information about the pre-connection (free) output format
	// is known, call this method.  this info may be useful in
	// finding the output to re-establish the connection later on.
			
	void setOutputHint(
		const char*								origName,
		const media_format&				origFormat);

	// if any information about the pre-connection (free) input format
	// is known, call this method.  this info may be useful in
	// finding the output to re-establish the connection later on.
			
	void setInputHint(
		const char*								origName,
		const media_format&				origFormat);
	
	// [e.moon 8dec99]
	void setRequestedFormat(
		const media_format&				reqFormat);
		
private:					// *** members

	// info that may be useful for reconstituting a particular
	// connection later on.
	struct endpoint_hint {
		endpoint_hint(const char* _name, const media_format& _format) :
			name(_name), format(_format) {}

		BString					name;
		media_format		format;
	};

	// waiting to die?
	bool												m_disconnected;

	// unique connection ID
	uint32											m_id;

	// [e.moon 14oct99] now stores media_nodes

	// source/output info
	media_node									m_sourceNode;
	media_source								m_source;
	BString											m_outputName;

	endpoint_hint*							m_outputHint;	
	
	// dest/input info
	media_node									m_destinationNode;
	media_destination						m_destination;
	BString											m_inputName;
	
	endpoint_hint*							m_inputHint;
	
	// connection format
	media_format								m_format;

	// behaviour modification	
	uint32											m_flags;

	// [e.moon 8dec99] initial requested format
	media_format								m_requestedFormat;
};

__END_CORTEX_NAMESPACE

#endif /*__Connection_H__*/
