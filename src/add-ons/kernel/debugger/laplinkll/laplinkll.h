/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 * 
 * Copyright 2005, François Revol.
 */

/*
	Description: Implements a tty on top of the parallel port, 
				 using PLIP-like byte-by-byte protocol.
				 Low level stuff.
*/


// LPT1
#define LPTBASE 0x378
#define LPTIRQ 7
#define LPTDONGLE "/dev/misc/dongle/parallel1"
#define LPTSPIN 30
#define LPTSOFTRIES 1000
#define LPTNIBTRIES 100
#define LAPLINK_MAX_FRAME 512

typedef struct laplink_state {
	int32 port;
	int32 irq;
	
	uint8 rstate;
	uint8 wstate;
} laplink_state;

// framing
extern status_t ll_send_sof(laplink_state *st);
extern status_t ll_check_sof(laplink_state *st);
extern status_t ll_ack_sof(laplink_state *st);
extern status_t ll_send_eof(laplink_state *st);

// nibbles
extern status_t ll_send_lnibble(laplink_state *st, uint8 v);
extern status_t ll_send_mnibble(laplink_state *st, uint8 v);
extern status_t ll_wait_lnibble(laplink_state *st, uint8 *v);
extern status_t ll_wait_mnibble(laplink_state *st, uint8 *v);

// byte mode
extern status_t ll_send_byte(laplink_state *st, uint8 v);
extern status_t ll_check_byte(laplink_state *st, uint8 *v);
extern status_t ll_wait_byte(laplink_state *st, uint8 *v);

// frame mode
extern status_t ll_send_frame(laplink_state *st, const uint8 *buff, size_t *len);
extern status_t ll_check_frame(laplink_state *st, uint8 *buff, size_t *len);
extern status_t ll_wait_frame(laplink_state *st, uint8 *buff, size_t *len);


