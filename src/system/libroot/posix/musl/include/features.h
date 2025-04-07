#ifndef _MUSL_FEATURES_H
#define _MUSL_FEATURES_H

#define weak __attribute__((__weak__))
#define hidden __attribute__((__visibility__("hidden")))
#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

#if __GNUC__ < 4
#define restrict
#endif

#endif
