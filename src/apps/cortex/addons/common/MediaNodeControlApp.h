// MediaMediaNodeControlApp.h
//
// TO DO
//   +++++ shut down automatically when node is deleted
//
// PURPOSE
//   Display a generic control panel for a given media node.
//
// HISTORY
//   e.moon 8jun99		Begun

#ifndef __MediaNodeControlApp_H__
#define __MediaNodeControlApp_H__

#include <Application.h>
#include <MediaNode.h>

class MediaNodeControlApp :
	public		BApplication {
	typedef	BApplication _inherited;

public:						// ctor/dtor
	virtual ~MediaNodeControlApp();
	MediaNodeControlApp(
		const char* pAppSignature,
		media_node_id nodeID);
		
private:						// members
	media_node			m_node;
};

#endif /*__MediaNodeControlApp_H__*/