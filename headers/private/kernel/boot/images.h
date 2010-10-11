/*
 * Copyright 2010, Philippe Houdoin <phoudoin at haiku-os dot org>.
 * Distributed under the terms of the MIT License.
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


