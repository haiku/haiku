// TransportWindow.h
//
// * PURPOSE
//   Standalone window container for the group/node inspectors.
// * HISTORY
//   e.moon		18aug99		Begun

#ifndef __TransportWindow_H__
#define __TransportWindow_H__

#include <Window.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TransportView;
class NodeManager;
class RouteWindow;

class TransportWindow :
	public	BWindow {
	typedef	BWindow _inherited;
	
public:											// *** messages
	enum message_t {
		// groupID: int32
		M_SELECT_GROUP					= TransportWindow_message_base
	};
	
public:											// *** ctors/dtor
	virtual ~TransportWindow();
	
	TransportWindow(
		NodeManager*						manager,
		BWindow*								parent,
		const char*							name);
		
	BMessenger parentWindow() const;

public:											// *** BHandler
	virtual void MessageReceived(
		BMessage*								message);

public:											// *** BWindow		
	virtual bool QuitRequested();
	
private:
	TransportView*						m_view;
	BWindow*									m_parent;

	void _togglePlayback(); //nyi
};

__END_CORTEX_NAMESPACE
#endif /*__TransportWindow_H__*/
