// cortex::NodeManager::AddOnHost.h
// * PURPOSE
//   Provides an interface to a separate BApplication whose
//   single responsibility is to launch nodes.
//   NodeManager-launched nodes now run in a separate team,
//   helping to lower the likelihood of a socially maladjusted
//   young node taking you out.
//
// * HISTORY
//   e.moon			6nov99

#ifndef __NodeManager_AddOnHost_H__
#define __NodeManager_AddOnHost_H__

#include <Application.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class AddOnHost {

public:												// *** static interface

	static status_t FindInstance(
		BMessenger*								outMessenger);

	static status_t Kill(
		bigtime_t									timeout=B_INFINITE_TIMEOUT);

	// returns B_NOT_ALLOWED if an instance has already been launched
	static status_t Launch(
		BMessenger*								outMessenger=0);
		
	static status_t InstantiateDormantNode(
		const dormant_node_info&	info,
		media_node*								outNode,
		bigtime_t									timeout=B_INFINITE_TIMEOUT);
		
	static status_t ReleaseInternalNode(
		const live_node_info&			info,
		bigtime_t									timeout=B_INFINITE_TIMEOUT);
		
	static BMessenger						s_messenger;
		
private:											// implementation
	friend class _AddOnHostApp;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeManager_AddOnHost_H__*/