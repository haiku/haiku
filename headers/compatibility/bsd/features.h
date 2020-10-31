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


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
	#define _ISOC11_SOURCE
#endif


#endif // _FEATURES_H
