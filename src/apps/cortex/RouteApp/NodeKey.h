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
