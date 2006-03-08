// AddOnHostProtocol.h
// * PURPOSE
//   contains all definitions needed for communications between
//   cortex/route and the add-on host app.
// * HISTORY
//   e.moon  02apr00   begun

#if !defined(__CORTEX_ADD_ON_HOST_PROTOCOL_H__)
#define __CORTEX_ADD_ON_HOST_PROTOCOL_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE
namespace addon_host {

const char* const g_appSignature = "application/x-vnd.Cortex.AddOnHost";

enum message_t {
	// inbound (sends reply)
	// 'info' B_RAW_TYPE (dormant_node_info)
	M_INSTANTIATE							= AddOnHostApp_message_base,

	// outbound
	// 'node_id' int32 (media_node_id)
	M_INSTANTIATE_COMPLETE,
	// 'error' int32
	M_INSTANTIATE_FAILED,
	
	// inbound (sends reply)
	// 'info' B_RAW_TYPE (live_node_info)
	M_RELEASE,
	
	// outbound
	// 'node_id' int32
	M_RELEASE_COMPLETE,
	// 'error' int32
	M_RELEASE_FAILED
};

}; // addon_host
__END_CORTEX_NAMESPACE
#endif //__CORTEX_ADD_ON_HOST_PROTOCOL_H__