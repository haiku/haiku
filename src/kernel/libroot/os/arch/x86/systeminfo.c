/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>


// this declaration could be placed in a header file somewhere, but it doesn't
// appear likely that any user functions besides 'get_cpuid()' would need it...
void cpuid(unsigned int selector, unsigned int *data);


/*
 *  get_cpuid() -- uses the 'cpuid' instruction to retrieve information
 *  about the computer make & model and the capabilities of the cpu(s)
 *	...this means the function is strictly INTEL PLATFORM ONLY!!!
 *
 *  parameters:
 *            info: out - the structure to hold the cpuid information
 *    eax_register: in  - the value to place in eax before invoking 'cpuid'
 *         cpu_num: in  - the number of the cpu to gather information about
 *                        (currently not used)
 *
 *  Note: the stage2 bootloader already verifies at startup time that the
 *  'cpuid' instruction can be called by the host computer, so no checking
 *  for this is done here.
 */

status_t
get_cpuid(cpuid_info *info, uint32 eax_register, uint32 cpu_num)
{
	unsigned int data[4];
	
	// call the instruction
	cpuid(eax_register, data);
	
	// fill the info struct with the return values
	info->regs.eax = data[0];
	info->regs.ebx = data[1];
	info->regs.ecx = data[2];
	info->regs.edx = data[3];
	
	if (eax_register == 0) {
		// fixups for this special case
		char *vendor = info->eax_0.vendorid;
		
		// the high-order word is non-zero on some Cyrix CPUs
		info->eax_0.max_eax = data[0] & 0xffff;
		
		// the vendor string bytes need a little re-ordering
		*((unsigned int *) &vendor[0]) = data[1];
		*((unsigned int *) &vendor[4]) = data[3];
		*((unsigned int *) &vendor[8]) = data[2];
	}
	
	return B_OK;
}

