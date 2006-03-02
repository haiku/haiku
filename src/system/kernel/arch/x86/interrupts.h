/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_INTERRUPTS_H
#define _KERNEL_ARCH_x86_INTERRUPTS_H


#ifdef __cplusplus
extern "C" {
#endif

void trap0();void trap1();void trap2();void trap3();void trap4();void trap5();
void trap6();void trap7();void trap9();void trap10();void trap11();
void trap12();void trap13();void trap14();void trap16();void trap17();void trap18();
void trap19();
void trap32();void trap33();void trap34();void trap35();void trap36();void trap37();
void trap38();void trap39();void trap40();void trap41();void trap42();void trap43();
void trap44();void trap45();void trap46();void trap47();

void double_fault();	// int 8

void trap99();

void trap251();void trap252();void trap253();void trap254();void trap255();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_x86_INTERRUPTS_H */
