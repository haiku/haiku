/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin, phoudoin at haiku-os dot org
 */
#ifndef KERNEL_BOOT_IMAGES_H
#define KERNEL_BOOT_IMAGES_H

#ifdef HAIKU_DISTRO_COMPATIBILITY_OFFICIAL
#	ifdef HAIKU_OFFICIAL_RELEASE
#		include <boot/images-tm.h>
#	else
#		include <boot/images-tm-development.h>
#	endif
#else
#	include <boot/images-sans-tm.h>
#endif

#endif	/* KERNEL_BOOT_ARCH_H */
