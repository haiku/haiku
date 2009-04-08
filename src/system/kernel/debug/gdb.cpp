/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/** Contains the code to interface with a remote GDB */

#include "gdb.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <ByteOrder.h>

#include <arch/debug_console.h>
#include <debug.h>
#include <smp.h>
#include <vm.h>


enum { INIT = 0, CMDREAD, CKSUM1, CKSUM2, WAITACK, QUIT, GDBSTATES };


static char sCommand[512];
static int sCommandIndex;
static int sCheckSum;

static char sReply[512];
static char sSafeMemory[512];


// utility functions


static int
parse_nibble(int input)
{
	int nibble = 0xff;

	if (input >= '0' && input <= '9')
		nibble = input - '0';

	if (input >= 'A' && input <= 'F')
		nibble = 0x0a + input - 'A';

	if (input >= 'a' && input <= 'f')
		nibble = 0x0a + input - 'a';

	return nibble;
}


//	#pragma mark - GDB protocol


static void
gdb_ack(void)
{
	arch_debug_serial_putchar('+');
}


static void
gdb_nak(void)
{
	arch_debug_serial_putchar('-');
}


static void
gdb_resend_reply(void)
{
	arch_debug_serial_puts(sReply);
}


static void
gdb_reply(char const* format, ...)
{
	int i;
	int len;
	int sum;
	va_list args;

	va_start(args, format);
	sReply[0] = '$';
	vsprintf(sReply + 1, format, args);
	va_end(args);

	len = strlen(sReply);
	sum = 0;
	for (i = 1; i < len; i++) {
		sum += sReply[i];
	}
	sum %= 256;

	sprintf(sReply + len, "#%02x", sum);

	gdb_resend_reply();
}


static void
gdb_regreply(int const* regs, int numregs)
{
	int i;
	int len;
	int sum;

	sReply[0] = '$';
	for (i = 0; i < numregs; i++)
		sprintf(sReply + 1 + 8 * i, "%08lx", B_HOST_TO_BENDIAN_INT32(regs[i]));

	len = strlen(sReply);
	sum = 0;
	for (i = 1; i < len; i++)
		sum += sReply[i];
	sum %= 256;

	sprintf(sReply + len, "#%02x", sum);

	gdb_resend_reply();
}


static void
gdb_memreply(char const* bytes, int numbytes)
{
	int i;
	int len;
	int sum;

	sReply[0] = '$';
	for (i = 0; i < numbytes; i++)
		sprintf(sReply + 1 + 2 * i, "%02x", (uint8)bytes[i]);

	len = strlen(sReply);
	sum = 0;
	for (i = 1; i < len; i++)
		sum += sReply[i];
	sum %= 256;

	sprintf(sReply + len, "#%02x", sum);

	gdb_resend_reply();
}


//	#pragma mark - checksum verification


static int
gdb_verify_checksum(void)
{
	int i;
	int len;
	int sum;

	len = strlen(sCommand);
	sum = 0;
	for (i = 0; i < len; i++)
		sum += sCommand[i];
	sum %= 256;

	return (sum == sCheckSum) ? 1 : 0;
}


//	#pragma mark - command parsing


static int
gdb_parse_command(void)
{
	if (!gdb_verify_checksum()) {
		gdb_nak();
		return INIT;
	} else
		gdb_ack();

	switch (sCommand[0]) {
		case 'H':
			// Command H (actually Hct) is used to select
			// the current thread (-1 meaning all threads)
			// We just fake we recognize the the command
			// and send an 'OK' response.
			gdb_reply("OK");
			break;

		case 'q':
			{
				extern unsigned __data_start;
				extern unsigned __bss_start;

				// There are several q commands:
				//
				//     qXXXX        Request info about XXXX.
				//     QXXXX=yyyy   Set value of XXXX to yyyy.
				//     qOffsets     Get segment offsets
				//
				// Currently we only support the 'qOffsets'
				// form.
				//
				// *Note* that we actually have to lie,
				// At first thought looks like we should
				// return '_start', '__data_start' &
				// '__bss_start', however gdb gets
				// confused because the kernel link script
				// pre-links at 0x80000000. To keep gdb
				// gdb happy we just substract that amount.
				if (strcmp(sCommand + 1, "Offsets") == 0) {
					gdb_reply("Text=%x;Data=%x;Bss=%x", 0,
						((unsigned)(&__data_start)) - 0x80000000,
						((unsigned)(&__bss_start)) - 0x80000000);
				} else
					gdb_reply("ENS");
			}
			break;

		case '?':
			// command '?' is used for retrieving the signal
			// that stopped the program. Fully implemeting
			// this command requires help from the debugger,
			// by now we just fake a SIGKILL
			gdb_reply("S09");	/* SIGKILL = 9 */
			break;

		case 'g':
			{
				int cpu;

				// command 'g' is used for reading the register
				// file. Faked by now.
				//
				// For x86 the register order is:
				//
				//    eax, ebx, ecx, edx,
				//    esp, ebp, esi, edi,
				//    eip, eflags,
				//    cs, ss, ds, es
				//
				// Note that even thought the segment descriptors
				// are actually 16 bits wide, gdb requires them
				// as 32 bit integers. Note also that for some
				// reason (unknown to me) gdb wants the register
				// dump in *big endian* format.
				cpu = smp_get_current_cpu();
				gdb_regreply(dbg_register_file[cpu], 14);
			}
			break;

		case 'm':
			{
				char* ptr;
				unsigned address;
				unsigned len;

				// The 'm' command has the form mAAA,LLL
				// where AAA is the address and LLL is the
				// number of bytes.
				ptr = sCommand + 1;
				address = 0;
				len = 0;
				while (ptr && *ptr && (*ptr != ',')) {
					address <<= 4;
					address += parse_nibble(*ptr);
					ptr += 1;
				}
				if (*ptr == ',')
					ptr += 1;

				while (ptr && *ptr) {
					len <<= 4;
					len += parse_nibble(*ptr);
					ptr += 1;
				}

				if (len > 128)
					len = 128;

				// We cannot directly access the requested memory
				// for gdb may be trying to access an stray pointer
				// We copy the memory to a safe buffer using
				// the bulletproof user_memcpy().
				if (user_memcpy(sSafeMemory, (char*)address, len) < 0)
					gdb_reply("E02");
				else
					gdb_memreply(sSafeMemory, len);
			}
			break;

		case 'k':
			// Command 'k' actual semantics is 'kill the damn thing'.
			// However gdb sends that command when you disconnect
			// from a debug session. I guess that 'kill' for the
			// kernel would map to reboot... however that's a
			// a very mean thing to do, instead we just quit
			// the gdb state machine and fallback to the regular
			// kernel debugger command prompt.
			return QUIT;

		default:
			gdb_reply("E01");
			break;
	}

	return WAITACK;
}


//	#pragma mark - protocol state machine


static int
gdb_init_handler(int input)
{
	switch (input) {
		case '$':
			memset(sCommand, 0, sizeof(sCommand));
			sCommandIndex = 0;
			return CMDREAD;

		default:
#if 0
			gdb_nak();
#else
			// looks to me like we should send
			// a NAK here but it kinda works
			// better if we just gobble all
			// junk chars silently
#endif
			return INIT;
	}
}


static int
gdb_cmdread_handler(int input)
{
	switch (input) {
		case '#':
			return CKSUM1;

		default:
			sCommand[sCommandIndex] = input;
			sCommandIndex += 1;
			return CMDREAD;
	}
}


static int
gdb_cksum1_handler(int input)
{
	int nibble = parse_nibble(input);

	if (nibble == 0xff) {
#if 0
		gdb_nak();
		return INIT;
#else
		// looks to me like we should send
		// a NAK here but it kinda works
		// better if we just gobble all
		// junk chars silently
#endif
	}

	sCheckSum = nibble << 4;

	return CKSUM2;
}


static int
gdb_cksum2_handler(int input)
{
	int nibble = parse_nibble(input);

	if (nibble == 0xff) {
#if 0
		gdb_nak();
		return INIT;
#else
		// looks to me like we should send
		// a NAK here but it kinda works
		// better if we just gobble all
		// junk chars silently
#endif
	}

	sCheckSum += nibble;

	return gdb_parse_command();
}


static int
gdb_waitack_handler(int input)
{
	switch (input) {
		case '+':
			return INIT;
		case '-':
			gdb_resend_reply();
			return WAITACK;

		default:
			// looks like gdb and us are out of sync,
			// send a NAK and retry from INIT state.
			gdb_nak();
			return INIT;
	}
}


static int
gdb_quit_handler(int input)
{
	(void)(input);

	// actually we should never be here
	return QUIT;
}


static int (*dispatch_table[GDBSTATES])(int) = {
	&gdb_init_handler,
	&gdb_cmdread_handler,
	&gdb_cksum1_handler,
	&gdb_cksum2_handler,
	&gdb_waitack_handler,
	&gdb_quit_handler
};


static int
gdb_state_dispatch(int curr, int input)
{
	if (curr < INIT || curr >= GDBSTATES)
		return QUIT;

	return dispatch_table[curr](input);
}


static int
gdb_state_machine(void)
{
	int state = INIT;
	int c;

	while (state != QUIT) {
		c = arch_debug_serial_getchar();
		state = gdb_state_dispatch(state, c);
	}

	return 0;
}


//	#pragma mark -


int
cmd_gdb(int argc, char** argv)
{
	(void)(argc);
	(void)(argv);

	return gdb_state_machine();
}
