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
void trap12();void trap13();void trap14();void trap16();void trap17();
void trap18();void trap19();

void trap32();void trap33();void trap34();void trap35();void trap36();
void trap37();void trap38();void trap39();void trap40();void trap41();
void trap42();void trap43();void trap44();void trap45();void trap46();
void trap47();void trap48();void trap49();void trap50();void trap51();
void trap52();void trap53();void trap54();void trap55();void trap56();
void trap57();void trap58();void trap59();void trap60();void trap61();
void trap62();void trap63();void trap64();void trap65();void trap66();
void trap67();void trap68();void trap69();void trap70();void trap71();
void trap72();void trap73();void trap74();void trap75();void trap76();
void trap77();void trap78();void trap79();void trap80();void trap81();
void trap82();void trap83();void trap84();void trap85();void trap86();
void trap87();void trap88();void trap89();void trap90();void trap91();
void trap92();void trap93();void trap94();void trap95();void trap96();
void trap97();

void double_fault();	// int 8
void trap14_double_fault();

void trap98();
void trap99();

void trap100();void trap101();void trap102();void trap103();void trap104();
void trap105();void trap106();void trap107();void trap108();void trap109();
void trap110();void trap111();void trap112();void trap113();void trap114();
void trap115();void trap116();void trap117();void trap118();void trap119();
void trap120();void trap121();void trap122();void trap123();void trap124();
void trap125();void trap126();void trap127();void trap128();void trap129();
void trap130();void trap131();void trap132();void trap133();void trap134();
void trap135();void trap136();void trap137();void trap138();void trap139();
void trap140();void trap141();void trap142();void trap143();void trap144();
void trap145();void trap146();void trap147();void trap148();void trap149();
void trap150();void trap151();void trap152();void trap153();void trap154();
void trap155();void trap156();void trap157();void trap158();void trap159();
void trap160();void trap161();void trap162();void trap163();void trap164();
void trap165();void trap166();void trap167();void trap168();void trap169();
void trap170();void trap171();void trap172();void trap173();void trap174();
void trap175();void trap176();void trap177();void trap178();void trap179();
void trap180();void trap181();void trap182();void trap183();void trap184();
void trap185();void trap186();void trap187();void trap188();void trap189();
void trap190();void trap191();void trap192();void trap193();void trap194();
void trap195();void trap196();void trap197();void trap198();void trap199();
void trap200();void trap201();void trap202();void trap203();void trap204();
void trap205();void trap206();void trap207();void trap208();void trap209();
void trap210();void trap211();void trap212();void trap213();void trap214();
void trap215();void trap216();void trap217();void trap218();void trap219();
void trap220();void trap221();void trap222();void trap223();void trap224();
void trap225();void trap226();void trap227();void trap228();void trap229();
void trap230();void trap231();void trap232();void trap233();void trap234();
void trap235();void trap236();void trap237();void trap238();void trap239();
void trap240();void trap241();void trap242();void trap243();void trap244();
void trap245();void trap246();void trap247();void trap248();void trap249();
void trap250();

void trap251();void trap252();void trap253();void trap254();void trap255();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_x86_INTERRUPTS_H */
