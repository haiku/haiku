/*
 * Copyright 2008, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_ARCH_ARM_ARCH_ARM_VERSION_H
#define SYSTEM_ARCH_ARM_ARCH_ARM_VERSION_H

/* map all the various ARM defines to arch version to allow checking for minimal arch */

#if defined(__ARM_ARCH_2__) || defined(__ARM_ARCH_3__)
#error we do not support this
#endif

#if defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__)
#define __ARM_ARCH__ 4
#endif

#if defined(__ARM_ARCH_5__) \
 || defined(__ARM_ARCH_5E__) \
 || defined(__ARM_ARCH_5T__) \
 || defined(__ARM_ARCH_5TE__) \
 || defined(__ARM_ARCH_5TEJ__)
#define __ARM_ARCH__ 5
#endif

#if defined(__ARM_ARCH_6__) \
 || defined(__ARM_ARCH_6J__) \
 || defined(__ARM_ARCH_6K__) \
 || defined(__ARM_ARCH_6ZK__) \
 || defined(__ARM_ARCH_6T2__)
#define __ARM_ARCH__ 6
#endif

#if defined(__ARM_ARCH_7__) \
 || defined(__ARM_ARCH_7A__) \
 || defined(__ARM_ARCH_7R__) \
 || defined(__ARM_ARCH_7M__)
#define __ARM_ARCH__ 7
#endif

#ifndef __ARM_ARCH__
#error cannot determine arm arch version
#endif

#endif	/* SYSTEM_ARCH_ARM_ARCH_ARM_VERSION_H */

