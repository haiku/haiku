// cortex::NodeManager::AddOnHostApp.h
// * PURPOSE
//   Definition of (and provisions for communication with)
//   a separate BApplication whose single responsibility is
//   to launch nodes.  NodeManager-launched nodes run in
//   another team, helping to lower the likelihood of a
//   socially maladjusted young node taking you out.
//
// * HISTORY
//   e.moon			6nov99

#ifndef __NodeManager_AddOnHostApp_H__
#define __NodeManager_AddOnHostApp_H__

#include <Application.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE
namespace addon_host {

class App :
	public	BApplication {
	typedef	BApplication _inherited;

public:											// *** implementation
	~App();
	App();

public:											// *** BLooper
	bool QuitRequested();
	
public:											// *** BHandler
	void MessageReceived(
		BMessage*								message);
		
private:										// implementation

};

}; // addon_host
__END_CORTEX_NAMESPACE
#endif /*__NodeManager_AddOnHostApp_H__*/