#ifndef DEBUG_PARANOIA_CONFIG_H
#define DEBUG_PARANOIA_CONFIG_H


// enable paranoia checks (0/1)
#ifndef ENABLE_PARANOIA_CHECKS
#	define ENABLE_PARANOIA_CHECKS 0
#endif

// number of check slots (should depend on the components for which checks
// are enabled)
#define PARANOIA_SLOT_COUNT	(32 * 1024)


// macros that set the paranoia check level for individual components
// levels:
// * PARANOIA_NAIVE (checks disabled)
// * PARANOIA_SUSPICIOUS
// * PARANOIA_OBSESSIVE
// * PARANOIA_INSANE

#define NET_BUFFER_PARANOIA		PARANOIA_NAIVE
#define OBJECT_CACHE_PARANOIA	PARANOIA_NAIVE


#endif	// DEBUG_PARANOIA_CONFIG_H
