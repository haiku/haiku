/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

typedef unsigned long ULONG;

ULONG	SCRIPT[] = {
	0x50000000L,	0x00000010L,
	0x98080000L,	0x00000011L,
	0x721A0000L,	0x00000000L,
	0x98080000L,	0x00000010L,
	0x43000000L,	0x00000040L,
	0x860B0000L,	0x00000038L,
	0x98080000L,	0x00000012L,
	0x98080000L,	0x00000013L,
	0x98080000L,	0x00000011L,
	0x42000000L,	0x00000050L,
	0x870B0000L,	0x00000088L,
	0x860A0000L,	0x000001D8L,
	0x820A0000L,	0x000001E8L,
	0x810A0000L,	0x000001F8L,
	0x800A0000L,	0x000001F8L,
	0x830A0000L,	0x00000200L,
	0x98080000L,	0x00000019L,
	0x1F000000L,	0x00000010L,
	0x800C0001L,	0x00000170L,
	0x800C0004L,	0x00000238L,
	0x800C0023L,	0x00000150L,
	0x60000040L,	0x00000000L,
	0x800C0002L,	0x00000050L,
	0x800C0007L,	0x00000050L,
	0x800C0003L,	0x00000050L,
	0x800C0080L,	0x00000050L,
	0x800C0081L,	0x00000050L,
	0x800C0082L,	0x00000050L,
	0x800C0083L,	0x00000050L,
	0x800C0084L,	0x00000050L,
	0x800C0085L,	0x00000050L,
	0x800C0086L,	0x00000050L,
	0x800C0087L,	0x00000050L,
	0x800C00C0L,	0x00000050L,
	0x800C00C1L,	0x00000050L,
	0x800C00C2L,	0x00000050L,
	0x800C00C3L,	0x00000050L,
	0x800C00C4L,	0x00000050L,
	0x800C00C5L,	0x00000050L,
	0x800C00C6L,	0x00000050L,
	0x800C00C7L,	0x00000050L,
	0x98080000L,	0x0000001AL,
	0x60000040L,	0x00000000L,
	0x1F000000L,	0x00000018L,
	0x60000040L,	0x00000000L,
	0x80080000L,	0x00000050L,
	0x60000040L,	0x00000000L,
	0x1F000000L,	0x00000018L,
	0x800C0003L,	0x000001B8L,
	0x800C0002L,	0x00000198L,
	0x98080000L,	0x0000001BL,
	0x60000040L,	0x00000000L,
	0x1F000000L,	0x00000038L,
	0x60000040L,	0x00000000L,
	0x98080000L,	0x0000001FL,
	0x60000040L,	0x00000000L,
	0x1F000000L,	0x00000020L,
	0x60000040L,	0x00000000L,
	0x98080000L,	0x0000001EL,
	0x1E000000L,	0x00000008L,
	0x80080000L,	0x00000050L,
	0x1A000000L,	0x00000030L,
	0x80080000L,	0x00000050L,
	0x98080000L,	0x00000017L,
	0x1B000000L,	0x00000028L,
	0x9F030000L,	0x00000016L,
	0x1F000000L,	0x00000010L,
	0x7C027F00L,	0x00000000L,
	0x60000040L,	0x00000000L,
	0x48000000L,	0x00000000L,
	0x98080000L,	0x00000014L,
	0x7C027F00L,	0x00000000L,
	0x60000040L,	0x00000000L,
	0x48000000L,	0x00000000L,
	0x98080000L,	0x00000015L,
	0x98080000L,	0x0000001CL

};

#define Abs_Count 22
char *Absolute_Names[Abs_Count] = {
	"ctxt_command",
	"ctxt_extdmsg",
	"ctxt_recvmsg",
	"ctxt_sendmsg",
	"ctxt_device",
	"ctxt_status",
	"ctxt_syncmsg",
	"ctxt_widemsg",
	"status_badextmsg",
	"status_badmsg",
	"status_badphase",
	"status_badstatus",
	"status_disconnect",
	"status_complete",
	"status_overrun",
	"status_ready",
	"status_reselected",
	"status_selected",
	"status_selftest",
	"status_syncin",
	"status_timeout",
	"status_widein"
};

#define A_ctxt_device	0x00000000L
ULONG A_ctxt_device_Used[] = {
	0x00000008L,
	0x00000012L
};

#define A_ctxt_sendmsg	0x00000008L
ULONG A_ctxt_sendmsg_Used[] = {
	0x00000077L
};

#define A_ctxt_recvmsg	0x00000010L
ULONG A_ctxt_recvmsg_Used[] = {
	0x00000023L,
	0x00000085L
};

#define A_status_ready	0x00000010L
ULONG A_status_ready_Used[] = {
	0x00000007L
};

#define A_status_reselected	0x00000011L
ULONG A_status_reselected_Used[] = {
	0x00000003L,
	0x00000011L
};

#define A_status_timeout	0x00000012L
ULONG A_status_timeout_Used[] = {
	0x0000000DL
};

#define A_status_selected	0x00000013L
ULONG A_status_selected_Used[] = {
	0x0000000FL
};

#define A_status_complete	0x00000014L
ULONG A_status_complete_Used[] = {
	0x0000008DL
};

#define A_status_disconnect	0x00000015L
ULONG A_status_disconnect_Used[] = {
	0x00000095L
};

#define A_status_badstatus	0x00000016L
ULONG A_status_badstatus_Used[] = {
	0x00000083L
};

#define A_status_overrun	0x00000017L
ULONG A_status_overrun_Used[] = {
	0x0000007FL
};

#define A_ctxt_extdmsg	0x00000018L
ULONG A_ctxt_extdmsg_Used[] = {
	0x00000057L,
	0x0000005FL
};

#define A_status_badphase	0x00000019L
ULONG A_status_badphase_Used[] = {
	0x00000021L
};

#define A_status_badmsg	0x0000001AL
ULONG A_status_badmsg_Used[] = {
	0x00000053L
};

#define A_status_badextmsg	0x0000001BL
ULONG A_status_badextmsg_Used[] = {
	0x00000065L
};

#define A_status_selftest	0x0000001CL
ULONG A_status_selftest_Used[] = {
	0x00000097L
};

#define A_status_syncin	0x0000001EL
ULONG A_status_syncin_Used[] = {
	0x00000075L
};

#define A_status_widein	0x0000001FL
ULONG A_status_widein_Used[] = {
	0x0000006DL
};

#define A_ctxt_syncmsg	0x00000020L
ULONG A_ctxt_syncmsg_Used[] = {
	0x00000071L
};

#define A_ctxt_status	0x00000028L
ULONG A_ctxt_status_Used[] = {
	0x00000081L
};

#define A_ctxt_command	0x00000030L
ULONG A_ctxt_command_Used[] = {
	0x0000007BL
};

#define A_ctxt_widemsg	0x00000038L
ULONG A_ctxt_widemsg_Used[] = {
	0x00000069L
};

#define Ent_do_dataout       	0x00000070L
#define Ent_do_datain        	0x00000068L
#define Ent_idle             	0x00000000L
#define Ent_phase_dataerr    	0x000001F8L
#define Ent_start            	0x00000020L
#define Ent_switch           	0x00000050L
#define Ent_switch_resel     	0x00000048L
#define Ent_test             	0x00000258L


ULONG	LABELPATCHES[] = {
	0x00000001L,
	0x00000009L,
	0x0000000BL,
	0x00000013L,
	0x00000015L,
	0x00000017L,
	0x00000019L,
	0x0000001BL,
	0x0000001DL,
	0x0000001FL,
	0x00000025L,
	0x00000027L,
	0x00000029L,
	0x0000002DL,
	0x0000002FL,
	0x00000031L,
	0x00000033L,
	0x00000035L,
	0x00000037L,
	0x00000039L,
	0x0000003BL,
	0x0000003DL,
	0x0000003FL,
	0x00000041L,
	0x00000043L,
	0x00000045L,
	0x00000047L,
	0x00000049L,
	0x0000004BL,
	0x0000004DL,
	0x0000004FL,
	0x00000051L,
	0x0000005BL,
	0x00000061L,
	0x00000063L,
	0x00000079L,
	0x0000007DL
};

ULONG	INSTRUCTIONS	= 0x0000004CL;
ULONG	PATCHES		= 0x00000025L;
