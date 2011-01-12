#ifndef _WRAPPER_H
#define _WRAPPER_H


#include <KernelExport.h>
#include <lock.h>


// debug output

#ifdef DEBUG_WAIT_ON_MSG
#	define DEBUG_WAIT snooze( DEBUG_WAIT_ON_MSG );
#else
#	define DEBUG_WAIT 
#endif

#ifdef DEBUG_WAIT_ON_ERROR
#	define DEBUG_WAIT_ERROR snooze( DEBUG_WAIT_ON_ERROR );
#else
#	define DEBUG_WAIT_ERROR
#endif

#ifndef DEBUG_MAX_LEVEL_FLOW
#	define DEBUG_MAX_LEVEL_FLOW 4
#endif

#ifndef DEBUG_MAX_LEVEL_INFO
#	define DEBUG_MAX_LEVEL_INFO 4
#endif

#ifndef DEBUG_MAX_LEVEL_ERROR
#	define DEBUG_MAX_LEVEL_ERROR 4
#endif

#ifndef DEBUG_MSG_PREFIX
#	define DEBUG_MSG_PREFIX ""
#endif

#ifndef debug_level_flow
#	define debug_level_flow 0
#endif

#ifndef debug_level_info
#	define debug_level_info 1
#endif

#ifndef debug_level_error
#	define debug_level_error 2
#endif

#define FUNC_NAME DEBUG_MSG_PREFIX, __FUNCTION__

#define SHOW_FLOW(seriousness, format, param...) \
	do { if( seriousness <= debug_level_flow && seriousness <= DEBUG_MAX_LEVEL_FLOW ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME, param ); DEBUG_WAIT \
	}} while( 0 )

#define SHOW_FLOW0(seriousness, format) \
	do { if( seriousness <= debug_level_flow && seriousness <= DEBUG_MAX_LEVEL_FLOW ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME); DEBUG_WAIT \
	}} while( 0 )

#define SHOW_INFO(seriousness, format, param...) \
	do { if( seriousness <= debug_level_info && seriousness <= DEBUG_MAX_LEVEL_INFO ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME, param ); DEBUG_WAIT \
	}} while( 0 )

#define SHOW_INFO0(seriousness, format) \
	do { if( seriousness <= debug_level_info && seriousness <= DEBUG_MAX_LEVEL_INFO ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME); DEBUG_WAIT \
	}} while( 0 )

#define SHOW_ERROR(seriousness, format, param...) \
	do { if( seriousness <= debug_level_error && seriousness <= DEBUG_MAX_LEVEL_ERROR ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME, param ); DEBUG_WAIT_ERROR \
	}} while( 0 )

#define SHOW_ERROR0(seriousness, format) \
	do { if( seriousness <= debug_level_error && seriousness <= DEBUG_MAX_LEVEL_ERROR ) { \
		dprintf( "%s%s: "format"\n", FUNC_NAME); DEBUG_WAIT_ERROR \
	}} while( 0 )


#endif	/* _WRAPPER_H */
