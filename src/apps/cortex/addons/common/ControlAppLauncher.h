// ControlAppLauncher.h
// * PURPOSE
//   A ControlAppLauncher manages a control-panel application
//   for a given Media Kit node.  It takes responsibility for
//   launching the application, maintaining a BMessenger to
//   it for control purposes, and telling it to quit upon
//   destruction.
//
// * HISTORY
//   e.moon		17jun99		Begun.

#ifndef __ControlAppLauncher_H__
#define __ControlAppLauncher_H__

#include <kernel/image.h>
#include <MediaDefs.h>
#include <Messenger.h>

class ControlAppLauncher {
public:						// ctor/dtor
	virtual ~ControlAppLauncher(); //nyi
	
	ControlAppLauncher(
		media_node_id mediaNode,
		bool launchNow=true); //nyi
				
public:						// accessors
	bool launched() const { return m_launched; }
	status_t error() const { return m_error; }
	const BMessenger& messenger() const { return m_messenger; }

public:						// operations
	status_t launch(); //nyi
	
private:						// guts
	void quit();

	bool							m_launched;
	status_t				m_error;
	BMessenger			m_messenger;
};

#endif /*__ControlAppLauncher_H__*/
