/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include "stage2_priv.h"

void system_reset_exception_entry();
extern int system_reset_exception_entry_end;
void machine_check_exception_entry();
extern int machine_check_exception_entry_end;
void dsi_exception_entry();
extern int dsi_exception_entry_end;
void isi_exception_entry();
extern int isi_exception_entry_end;
void external_interrupt_entry();
extern int external_interrupt_entry_end;
void alignment_exception_entry();
extern int alignment_exception_entry_end;
void program_exception_entry();
extern int program_exception_entry_end;
void decrementer_exception_entry();
extern int decrementer_exception_entry_end;
void system_call_exception_entry();
extern int system_call_exception_entry_end;
void trace_exception_entry();
extern int trace_exception_entry_end;
void floating_point_assist_exception_entry();
extern int floating_point_assist_exception_entry_end;

void s2_faults_init(kernel_args *ka)
{
	printf("s2_faults_init: entry\n");
	setmsr(getmsr() & ~0x00000040); // make it look for exceptions in the zero page
	printf("s2_faults_init: foo\n");
	printf("0x%x\n", *(unsigned int *)0x00000100);
	printf("0x%x\n", *(unsigned int *)0x00000104);
	printf("0x%x\n", *(unsigned int *)0x00000108);
	printf("0x%x\n", *(unsigned int *)0x0000010c);
	printf("%d\n", (int)&system_reset_exception_entry_end - (int)&system_reset_exception_entry);
	memcpy((void *)0x00000100, &system_reset_exception_entry,
		(int)&system_reset_exception_entry_end - (int)&system_reset_exception_entry);
	printf("%d\n",
		memcmp((void *)0x00000100, &system_reset_exception_entry,
		(int)&system_reset_exception_entry_end - (int)&system_reset_exception_entry));
	printf("0x%x\n", *(unsigned int *)0x00000100);
	printf("0x%x\n", *(unsigned int *)0x00000104);
	printf("0x%x\n", *(unsigned int *)0x00000108);
	printf("0x%x\n", *(unsigned int *)0x0000010c);
	memcpy((void *)0x00000200, &machine_check_exception_entry,
		(int)&machine_check_exception_entry_end - (int)&machine_check_exception_entry);
	memcpy((void *)0x00000300, &dsi_exception_entry,
		(int)&dsi_exception_entry_end - (int)&dsi_exception_entry);
	memcpy((void *)0x00000400, &isi_exception_entry,
		(int)&isi_exception_entry_end - (int)&isi_exception_entry);
	memcpy((void *)0x00000500, &external_interrupt_entry,
		(int)&external_interrupt_entry_end - (int)&external_interrupt_entry);
	memcpy((void *)0x00000600, &alignment_exception_entry,
		(int)&alignment_exception_entry_end - (int)&alignment_exception_entry);
	memcpy((void *)0x00000700, &program_exception_entry,
		(int)&program_exception_entry_end - (int)&program_exception_entry);
	memcpy((void *)0x00000900, &decrementer_exception_entry,
		(int)&decrementer_exception_entry_end - (int)&decrementer_exception_entry);
	memcpy((void *)0x00000c00, &system_call_exception_entry,
		(int)&system_call_exception_entry_end - (int)&system_call_exception_entry);
	memcpy((void *)0x00000d00, &trace_exception_entry,
		(int)&trace_exception_entry_end - (int)&trace_exception_entry);
	memcpy((void *)0x00000e00, &floating_point_assist_exception_entry,
		(int)&floating_point_assist_exception_entry_end - (int)&floating_point_assist_exception_entry);
	printf("s2_faults_init: exit\n");

	syncicache(0, 0x1000);
}

void system_reset_exception()
{
	printf("system_reset_exception\n");
	for(;;);
}

void machine_check_exception()
{
	printf("machine_check_exception\n");
	for(;;);
}

void dsi_exception()
{
	*(int *)0x16008190 = 0xffffff;
	printf("dsi_exception\n");
	for(;;);
}

void isi_exception()
{
	printf("isi_exception\n");
	for(;;);
}

void external_interrupt()
{
	printf("external_interrupt\n");
	for(;;);
}

void alignment_exception()
{
	printf("alignment_exception\n");
	for(;;);
}

void program_exception()
{
	printf("program_exception\n");
	for(;;);
}

void decrementer_exception()
{
	printf("decrementer_exception\n");
	for(;;);
}

void system_call_exception()
{
	printf("system_call_exception\n");
	for(;;);
}

void trace_exception()
{
	printf("trace_exception\n");
	for(;;);
}

void floating_point_assist_exception()
{
	printf("floating_point_assist_exception\n");
	for(;;);
}

