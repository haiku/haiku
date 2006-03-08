// cortex_defs.h
// * PURPOSE
//   Preprocessor stuff for the Cortex toolkit.
//
// * NOTES
//   To place the Cortex classes in a namespace of your
//   choosing, set the preprocessor variable CORTEX_NAMESPACE
//   to whatever string you want.
//
//   Message 'namespaces' are defined here as well.
//
// * HISTORY
//   e.moon		25jun99		Begun

#ifndef __cortex_defs_h__
#define __cortex_defs_h__

#include <SupportDefs.h>

// *** namespace support
#ifdef CORTEX_NAMESPACE
	#define __BEGIN_CORTEX_NAMESPACE namespace CORTEX_NAMESPACE {
	#define __END_CORTEX_NAMESPACE }
	#define __USE_CORTEX_NAMESPACE using namespace CORTEX_NAMESPACE;
	#define __CORTEX_NAMESPACE__ CORTEX_NAMESPACE::
#else
	#define CORTEX_NAMESPACE
	#define __BEGIN_CORTEX_NAMESPACE
	#define __END_CORTEX_NAMESPACE
	#define __USE_CORTEX_NAMESPACE
	#define __CORTEX_NAMESPACE__
#endif

#define TOUCH(x) ((void)(x))

// *** message 'what' code base values

const uint32 NodeManager_message_base				= 'NMaA';
const uint32 NodeManager_int_message_base		= 'Nm_A';
const uint32 NodeGroup_message_base					= 'NGrA';
const uint32 NodeRef_message_base						= 'NReA';
const uint32 NodeSyncThread_message_base		= 'NStA';

const uint32 RouteApp_message_base					= 'RoAA';
const uint32 RouteWindow_message_base				= 'RoWA';
const uint32 DiagramView_message_base				= 'DiVA';
const uint32 MediaRoutingView_message_base	= 'RoVA';

const uint32 TransportWindow_message_base		= 'TrWA';
const uint32 TransportView_message_base			= 'TrVA';

const uint32 ValControl_message_base				= 'VcnA';

const uint32 Observable_message_base				= 'ObTA';
const uint32 Observer_message_base					= 'Ob_A';

const uint32 AddOnHostApp_message_base 			= 'NahA';

const uint32 RouteAppNodeManager_message_base
																						= 'RMaA';																

const uint32 InfoView_message_base					= 'InVA';

const uint32 ParameterWindow_message_base		= 'PaWA';

#endif /*__cortex_defs_h__*/
