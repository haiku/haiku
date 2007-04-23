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
#ifndef NODE_MANAGER_ADD_ON_HOST_APP_H
#define NODE_MANAGER_ADD_ON_HOST_APP_H


#include "cortex_defs.h"

#include <Application.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>


__BEGIN_CORTEX_NAMESPACE
namespace addon_host {

class App:public BApplication {
	typedef	BApplication _inherited;

	public:
		~App();
		App();

	public:
		bool QuitRequested();
		void MessageReceived(BMessage* message);

};

}	// addon_host
__END_CORTEX_NAMESPACE

#endif	// NODE_MANAGER_ADD_ON_HOST_APP_H
