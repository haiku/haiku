#ifndef DEBUG_PARANOIA_CONFIG_H
#define DEBUG_PARANOIA_CONFIG_H


// enable paranoia checks (0/1)
#ifndef ENABLE_PARANOIA_CHECKS
#	define ENABLE_PARANOIA_CHECKS 1
#endif

// number of check slots (should depend on the components for which checks
// are enabled)
#define PARANOIA_SLOT_COUNT	(32 * 1024)


// macros that enable paranoia checks for individual components

//#define NET_BUFFER_PARANOIA	1


#endif	// DEBUG_PARANOIA_CONFIG_H
