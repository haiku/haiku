/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
.text
// void getibats(int bats[8]);
.globl getibats
getibats:
	mfibatu 0,0
	stw     0,0(3)
	mfibatl 0,0
	stwu	0,4(3)
	mfibatu 0,1
	stwu	0,4(3)
	mfibatl 0,1
	stwu	0,4(3)
	mfibatu 0,2
	stwu	0,4(3)
	mfibatl 0,2
	stwu	0,4(3)
	mfibatu 0,3
	stwu	0,4(3)
	mfibatl 0,3
	stwu	0,4(3)
	blr

// void setibats(int bats[8]);
.globl setibats
setibats:
	mfmsr	8
	li		0,0
	mtmsr	0

	lwz		0,0(3)
	mtibatu 0,0
	isync
	lwzu	0,4(3)
	mtibatl 0,0
	isync
	lwzu	0,4(3)
	mtibatu 1,0
	isync
	lwzu	0,4(3)
	mtibatl 1,0
	isync
	lwzu	0,4(3)
	mtibatu 2,0
	isync
	lwzu	0,4(3)
	mtibatl 2,0
	isync
	lwzu	0,4(3)
	mtibatu 3,0
	isync
	lwzu	0,4(3)
	mtibatl 3,0

	isync

	mtmsr	8
	isync
	blr

// void getdbats(int bats[8]);
.globl getdbats
getdbats:
	mfdbatu 0,0
	stw     0,0(3)
	mfdbatl 0,0
	stwu	0,4(3)
	mfdbatu 0,1
	stwu	0,4(3)
	mfdbatl 0,1
	stwu	0,4(3)
	mfdbatu 0,2
	stwu	0,4(3)
	mfdbatl 0,2
	stwu	0,4(3)
	mfdbatu 0,3
	stwu	0,4(3)
	mfdbatl 0,3
	stwu	0,4(3)
	blr

// void setdbats(int bats[8]);
.globl setdbats
setdbats:
	mfmsr	8
	li		0,0
	mtmsr	0

	lwz		0,0(3)
	mtdbatu 0,0
	lwzu	0,4(3)
	mtdbatl 0,0
	lwzu	0,4(3)
	mtdbatu 1,0
	lwzu	0,4(3)
	mtdbatl 1,0
	lwzu	0,4(3)
	mtdbatu 2,0
	lwzu	0,4(3)
	mtdbatl 2,0
	lwzu	0,4(3)
	mtdbatu 3,0
	lwzu	0,4(3)
	mtdbatl 3,0

	mtmsr	8
	sync
	blr

// unsigned int getsdr1();
.globl getsdr1
getsdr1:
	mfsdr1	3
	blr

// void setsdr1(unsigned int sdr);
.globl setsdr1
setsdr1:
	sync
	mtsdr1	3
	sync
	blr

// unsigned int getsr(int sr);
.globl getsr
getsr:
	mfsrin	3,3
	blr

// unsigned int getmsr();
.globl getmsr
getmsr:
	mfmsr 3
	blr

// void setmsr(unsigned int msr);
.globl setmsr
setmsr:
	mtmsr 3
	blr

.globl system_reset_exception_entry
system_reset_exception_entry:
	lis		3,system_reset_exception@ha
	ori		3,3,system_reset_exception@l
	mtlr	3
	blr
.globl system_reset_exception_entry_end
system_reset_exception_entry_end:

.globl machine_check_exception_entry
machine_check_exception_entry:
	lis		3,machine_check_exception@ha
	ori		3,3,machine_check_exception@l
	mtlr	3
	blr
.globl machine_check_exception_entry_end
machine_check_exception_entry_end:

.globl dsi_exception_entry
dsi_exception_entry:
	lis		3,dsi_exception@ha
	ori		3,3,dsi_exception@l
	mtlr	3
	blr
.globl dsi_exception_entry_end
dsi_exception_entry_end:

.globl isi_exception_entry
isi_exception_entry:
	lis		3,isi_exception@ha
	ori		3,3,isi_exception@l
	mtlr	3
	blr
.globl isi_exception_entry_end
isi_exception_entry_end:

.globl external_interrupt_entry
external_interrupt_entry:
	lis		3,external_interrupt@ha
	ori		3,3,external_interrupt@l
	mtlr	3
	blr
.globl external_interrupt_entry_end
external_interrupt_entry_end:

.globl alignment_exception_entry
alignment_exception_entry:
	lis		3,alignment_exception@ha
	ori		3,3,alignment_exception@l
	mtlr	3
	blr
.globl alignment_exception_entry_end
alignment_exception_entry_end:

.globl program_exception_entry
program_exception_entry:
	lis		3,program_exception@ha
	ori		3,3,program_exception@l
	mtlr	3
	blr
.globl program_exception_entry_end
program_exception_entry_end:

.globl decrementer_exception_entry
decrementer_exception_entry:
	lis		3,decrementer_exception@ha
	ori		3,3,decrementer_exception@l
	mtlr	3
	blr
.globl decrementer_exception_entry_end
decrementer_exception_entry_end:

.globl system_call_exception_entry
system_call_exception_entry:
	lis		3,system_call_exception@ha
	ori		3,3,system_call_exception@l
	mtlr	3
	blr
.globl system_call_exception_entry_end
system_call_exception_entry_end:

.globl trace_exception_entry
trace_exception_entry:
	lis		3,trace_exception@ha
	ori		3,3,trace_exception@l
	mtlr	3
	blr
.globl trace_exception_entry_end
trace_exception_entry_end:

.globl floating_point_assist_exception_entry
floating_point_assist_exception_entry:
	lis		3,floating_point_assist_exception@ha
	ori		3,3,floating_point_assist_exception@l
	mtlr	3
	blr
.globl floating_point_assist_exception_entry_end
floating_point_assist_exception_entry_end:
