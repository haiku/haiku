/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FEATURES_H
#define _FEATURES_H


#if defined(_BSD_SOURCE) \
	|| (!defined(__STRICT_ANSI__) && !defined(_POSIX_C_SOURCE))
	#define _DEFAULT_SOURCE
#endif


#endif // _FEATURES_H
