//------------------------------------------------------------------------------
//	Copyright (c) 2008, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		bootsplashstructs.h
//	Author:			Artur Wyszynski <harakash@gmail.com>
//	Description:	Boot splash header
//  
//------------------------------------------------------------------------------

#ifndef KERNEL_BOOT_SPLASH_H
#define KERNEL_BOOT_SPLASH_H

struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BOOT_SPLASH_STAGE_1_INIT_MODULES	= 0,
	BOOT_SPLASH_STAGE_2_BOOTSTRAP_FS,
	BOOT_SPLASH_STAGE_3_INIT_DEVICES,
	BOOT_SPLASH_STAGE_4_MOUNT_BOOT_FS,
	BOOT_SPLASH_STAGE_5_INIT_CPU_MODULES,
	BOOT_SPLASH_STAGE_6_INIT_VM_MODULES,
	BOOT_SPLASH_STAGE_7_RUN_BOOT_SCRIPT,

	BOOT_SPLASH_STAGE_MAX // keep this at the end
};

status_t boot_splash_fb_init(struct kernel_args *args);
bool boot_splash_fb_available(void);

void boot_splash_set_stage(int stage);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_BOOT_SPLASH_H */


