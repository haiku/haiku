/* Contains the code to interface with a remote GDB */

/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <vm.h>
#include <smp.h>
#include <debug.h>
#include <gdb.h>
#include <arch/dbg_console.h>


enum { INIT= 0, CMDREAD, CKSUM1, CKSUM2, WAITACK, QUIT, GDBSTATES };


static char cmd[512];
static int  cmd_ptr;
static int  checksum;

static char reply[512];

static char safe_mem[512];


/*
 * utility functions
 */
static
int
htons(short value)
{
	return ((value>>8)&0xff) | ((value&0xff)<<8);
}

static
int
htonl(int value)
{
	return htons((value>>16)&0xffff) | (htons(value&0xffff)<<16);
}

static
int
parse_nibble(int input)
{
	int nibble= 0xff;

	if((input>= '0') && (input<= '9')) {
		nibble= input-'0';
	}
	if((input>= 'A') && (input<= 'F')) {
		nibble= 0x0a+(input-'A');
	}
	if((input>= 'a') && (input<= 'f')) {
		nibble= 0x0a+(input-'a');
	}

	return nibble;
}



/*
 * GDB protocol ACK & NAK & Reply
 *
 */
static
void
gdb_ack(void)
{
	dbg_putch('+');
}

static
void
gdb_nak(void)
{
	dbg_putch('-');
}

static
void
gdb_resend_reply(void)
{
	dbg_puts(reply);
}

static
void
gdb_reply(char const *fmt, ...)
{
	int i;
	int len;
	int sum;
	va_list args;

	va_start(args, fmt);
	reply[0]= '$';
	vsprintf(reply+1, fmt, args);
	va_end(args);

	len= strlen(reply);
	sum= 0;
	for(i= 1; i< len; i++) {
		sum+= reply[i];
	}
	sum%= 256;

	sprintf(reply+len, "#%02x", sum);

	gdb_resend_reply();
}

static
void
gdb_regreply(int const *regs, int numregs)
{
	int i;
	int len;
	int sum;

	reply[0]= '$';
	for(i= 0; i< numregs; i++) {
		sprintf(reply+1+8*i, "%08x", htonl(regs[i]));
	}

	len= strlen(reply);
	sum= 0;
	for(i= 1; i< len; i++) {
		sum+= reply[i];
	}
	sum%= 256;

	sprintf(reply+len, "#%02x", sum);

	gdb_resend_reply();
}

static
void
gdb_memreply(char const *bytes, int numbytes)
{
	int i;
	int len;
	int sum;

	reply[0]= '$';
	for(i= 0; i< numbytes; i++) {
		sprintf(reply+1+2*i, "%02x", (unsigned char)bytes[i]);
	}

	len= strlen(reply);
	sum= 0;
	for(i= 1; i< len; i++) {
		sum+= reply[i];
	}
	sum%= 256;

	sprintf(reply+len, "#%02x", sum);

	gdb_resend_reply();
}



/*
 * checksum verification
 */
static
int
gdb_verify_checksum(void)
{
	int i;
	int len;
	int sum;

	len= strlen(cmd);
	sum= 0;
	for(i= 0; i< len; i++) {
		sum+= cmd[i];
	}
	sum%= 256;

	return (sum==checksum)?1:0;
}


/*
 * command parsing an dispatching
 */
static
int
gdb_parse_command(void)
{
//	int retval;

	if(!gdb_verify_checksum()) {
		gdb_nak();
		return INIT;
	} else {
		gdb_ack();
	}

	switch(cmd[0]) {

		case 'H':
			{
				/*
				 * Command H (actually Hct) is used to select
				 * the current thread (-1 meaning all threads)
				 * We just fake we recognize the the command
				 * and send an 'OK' response.
				 */
				gdb_reply("OK");
			} break;

		case 'q':
			{
//				extern unsigned _start;
				extern unsigned __data_start;
				extern unsigned __bss_start;

				/*
				 * There are several q commands:
				 *
				 *     qXXXX        Request info about XXXX.
				 *     QXXXX=yyyy   Set value of XXXX to yyyy.
				 *     qOffsets     Get segment offsets
				 *
				 * Currently we only support the 'qOffsets'
				 * form.
				 *
				 * *Note* that we actually have to lie,
				 * At first thought looks like we should
				 * return '_start', '__data_start' &
				 * '__bss_start', however gdb gets
				 * confused because the kernel link script
				 * pre-links at 0x80000000. To keep gdb
				 * gdb happy we just substract that amount.
				 */
				if(strcmp(cmd+1, "Offsets")== 0) {
					gdb_reply(
						"Text=%x;Data=%x;Bss=%x",
						0,
						((unsigned)(&__data_start))-0x80000000,
						((unsigned)(&__bss_start))-0x80000000
					);
				} else {
					gdb_reply("ENS");
				}
			} break;

		case '?':
			{
				/*
				 * command '?' is used for retrieving the signal
				 * that stopped the program. Fully implemeting
				 * this command requires help from the debugger,
				 * by now we just fake a SIGKILL
				 */
				gdb_reply("S09");	/* SIGKILL = 9 */
			} break;

		case 'g':
			{
				int cpu;

				/*
				 * command 'g' is used for reading the register
				 * file. Faked by now.
				 *
				 * For x86 the register order is:
				 *
				 *    eax, ebx, ecx, edx,
				 *    esp, ebp, esi, edi,
				 *    eip, eflags,
				 *    cs, ss, ds, es
				 *
				 * Note that even thought the segment descriptors
				 * are actually 16 bits wide, gdb requires them
				 * as 32 bit integers. Note also that for some
				 * reason (unknown to me) gdb wants the register
				 * dump in *big endian* format.
				 */
				cpu= smp_get_current_cpu();
				gdb_regreply(dbg_register_file[cpu], 14);
			} break;

		case 'm':
			{
				char *ptr;
				unsigned address;
				unsigned len;

				/*
				 * The 'm' command has the form mAAA,LLL
				 * where AAA is the address and LLL is the
				 * number of bytes.
				 */
				ptr= cmd+1;
				address= 0;
				len= 0;
				while(ptr && *ptr && (*ptr!= ',')) {
					address<<= 4;
					address+= parse_nibble(*ptr);
					ptr+= 1;
				}
				if(*ptr== ',') {
					ptr+= 1;
				}
				while(ptr && *ptr) {
					len<<= 4;
					len+= parse_nibble(*ptr);
					ptr+= 1;
				}

				if(len> 128) {
					len= 128;
				}

				/*
				 * We cannot directly access the requested memory
				 * for gdb may be trying to access an stray pointer
				 * We copy the memory to a safe buffer using
				 * the bulletproof user_memcpy().
				 */
				if(user_memcpy(safe_mem, (char*)address, len)< 0) {
					gdb_reply("E02");
				} else {
					gdb_memreply(safe_mem, len);
				}
			} break;

		case 'k':
			{
				/*
				 * Command 'k' actual semantics is 'kill the damn thing'.
				 * However gdb sends that command when you disconnect
				 * from a debug session. I guess that 'kill' for the
				 * kernel would map to reboot... however that's a
				 * a very mean thing to do, instead we just quit
				 * the gdb state machine and fallback to the regular
				 * kernel debugger command prompt.
				 */
				return QUIT;
			} break;

		default:
			{
				gdb_reply("E01");
			} break;
	}

	return WAITACK;
}



/*
 * GDB protocol state machine
 */
static
int
gdb_init_handler(int input)
{
	switch(input) {
		case '$':
			memset(cmd, 0, sizeof(cmd));
			cmd_ptr= 0;
			return CMDREAD;
		default:
#if 0
			gdb_nak();
#else
			/*
			 * looks to me like we should send
			 * a NAK here but it kinda works
			 * better if we just gobble all
			 * junk chars silently
			 */
#endif
			return INIT;
	}
}

static
int
gdb_cmdread_handler(int input)
{
	switch(input) {
		case '#':
			return CKSUM1;
		default:
			cmd[cmd_ptr]= input;
			cmd_ptr+= 1;
			return CMDREAD;
	}
}

static
int
gdb_cksum1_handler(int input)
{
	int nibble= parse_nibble(input);

	if(nibble== 0xff) {
#if 0
		gdb_nak();
		return INIT;
#else
		/*
		 * looks to me like we should send
		 * a NAK here but it kinda works
		 * better if we just gobble all
		 * junk chars silently
		 */
#endif
	}

	checksum= nibble<< 4;

	return CKSUM2;
}

static
int
gdb_cksum2_handler(int input)
{
	int nibble= parse_nibble(input);

	if(nibble== 0xff) {
#if 0
		gdb_nak();
		return INIT;
#else
		/*
		 * looks to me like we should send
		 * a NAK here but it kinda works
		 * better if we just gobble all
		 * junk chars silently
		 */
#endif
	}

	checksum+= nibble;

	return gdb_parse_command();
}

static
int
gdb_waitack_handler(int input)
{
	switch(input) {
		case '+':
			return INIT;
		case '-':
			gdb_resend_reply();
			return WAITACK;
		default:
			/*
			 * looks like gdb and us are out of synch,
			 * send a NAK and retry from INIT state.
			 */
			gdb_nak();
			return INIT;
	}
}

static
int
gdb_quit_handler(int input)
{
	(void)(input);

	/*
	 * actually we should never be here
	 */
	return QUIT;
}

static int (*dispatch_table[GDBSTATES])(int)=
{
	&gdb_init_handler,
	&gdb_cmdread_handler,
	&gdb_cksum1_handler,
	&gdb_cksum2_handler,
	&gdb_waitack_handler,
	&gdb_quit_handler
};

static
int
gdb_state_dispatch(int curr, int input)
{
	if(curr< INIT) {
		return QUIT;
	}
	if(curr>= GDBSTATES) {
		return QUIT;
	}

	return dispatch_table[curr](input);
}

static
int
gdb_state_machine(void)
{
	int state= INIT;
	int c;

	while(state!= QUIT) {
		c= arch_dbg_con_read();
		state= gdb_state_dispatch(state, c);
	}

	return 0;
}

void
cmd_gdb(int argc, char **argv)
{
	(void)(argc);
	(void)(argv);

	gdb_state_machine();
}
