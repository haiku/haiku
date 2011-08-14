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


// NodeKey.h
// * PURPOSE +++++ SUPERCEDED BY LiveNodeIO 20dec99 +++++
//
//   An IPersistent implementation that stores a string
//   identifier (key) for a media node whose description
//   is being imported from or exported to a stream.
//
//   A NodeKey may refer to a user-created node or one
//   of the following system-defined nodes:
//
//   AUDIO_INPUT
//   AUDIO_OUTPUT
//   AUDIO_MIXER
//   VIDEO_INPUT
//   VIDEO_OUTPUT
//   DAC_TIME_SOURCE
//   SYSTEM_TIME_SOURCE
//
// * HISTORY
//   e.moon			7dec99		Begun

#ifndef __NodeKey_H__
#define __NodeKey_H__

#include <MediaDefs.h>
#include "StringContent.h"

#include "cortex_defs.h"

__BEGIN_CORTEX_NAMESPACE

class NodeKey :
	public	StringContent {
	
public:														// content access

	const char* key() const { return content.String(); }

	// if the corresponding node doesn't exist, returns 0
	media_node_id node() const { return m_node; }

public:														// *** dtor, default ctors
	virtual ~NodeKey() {}
	NodeKey() : m_node(0) {}
	NodeKey(
		const NodeKey&								clone) { _clone(clone); }
	NodeKey& operator=(
		const NodeKey&								clone) { _clone(clone); return *this; }
		
	bool operator<(
		const NodeKey&								other) const {
		return key() < other.key();
	}

public:														// *** IPersistent

	// EXPORT
	
	virtual void xmlExportBegin(
		ExportContext&								context) const;
		
	virtual void xmlExportAttributes(
		ExportContext&								context) const;

	virtual void xmlExportContent(
		ExportContext&								context) const;
	
	virtual void xmlExportEnd(
		ExportContext&								context) const;


private:
	friend class RouteAppNodeManager;
	
	NodeKey(
		const char*										_key,
		media_node_id									_node) :
		StringContent(_key),
		m_node(_node) {}

private:
	inline void _clone(
		const NodeKey&								clone) {
		content = clone.content;
		m_node = clone.m_node;
	}

	media_node_id										m_node;
	
	static const char* const				s_element;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeKey_H__ */
