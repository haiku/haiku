/*
 *	Copyright 2010, Haiku Inc. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#ifndef _TTY_MODULE_H
#define _TTY_MODULE_H

#include <module.h>
#include <termios.h>
#include <Select.h>

struct tty;
struct tty_cookie;

typedef bool (*tty_service_func)(struct tty *tty, uint32 op, void *buffer,
	size_t length);

// flags
#define	TTYCARRIER		(1 << 0)
#define	TTYWRITABLE		(1 << 1)
#define	TTYWRITING		(1 << 2)
#define	TTYREADING		(1 << 3)
#define	TTYOSTOPPED		(1 << 4)
#define	TTYEXCLUSIVE	(1 << 5)
#define	TTYHWDCD		(1 << 6)
#define	TTYHWCTS		(1 << 7)
#define	TTYHWDSR		(1 << 8)
#define	TTYHWRI			(1 << 9)
#define	TTYFLOWFORCED	(1 << 10)

// ops
#define	TTYENABLE		0	/* bool enabled */
#define	TTYSETMODES		1	/* struct termios termios */
#define	TTYOSTART		2
#define	TTYOSYNC		3
#define	TTYISTOP		4	/* bool stopInput */
#define	TTYSETBREAK		5	/* bool break */
#define	TTYSETDTR		6	/* bool dataTerminalReady */
#define TTYSETRTS		7	/* bool requestToSend */
#define	TTYGETSIGNALS	8	/* call tty_hardware_signal for all bits */
#define TTYFLUSH		9	/* clear input and/or output buffers */

typedef struct tty_module_info tty_module_info;

struct tty_module_info {
	module_info	mi;

	struct tty *(*tty_create)(tty_service_func serviceFunction, bool isMaster);
	void		(*tty_destroy)(struct tty *tty);

	struct tty_cookie *
				(*tty_create_cookie)(struct tty *masterTTY, struct tty *slaveTTY,
					uint32 openMode);
	void		(*tty_close_cookie)(struct tty_cookie *cookie);
	void		(*tty_destroy_cookie)(struct tty_cookie *cookie);

	status_t	(*tty_read)(struct tty_cookie *cookie, void *_buffer,
					size_t *_length);
	status_t	(*tty_write)(struct tty_cookie *cookie, const void *buffer,
					size_t *length);
	status_t	(*tty_control)(struct tty_cookie *cookie, uint32 op,
					void *buffer, size_t length);
	status_t	(*tty_select)(struct tty_cookie *cookie, uint8 event,
					uint32 ref, selectsync *sync);
	status_t	(*tty_deselect)(struct tty_cookie *cookie, uint8 event,
					selectsync *sync);

	status_t	(*tty_hardware_signal)(struct tty_cookie *cookie,
					int signal, bool);
};

#define B_TTY_MODULE_NAME			"generic/tty/v1"

#endif /* _TTY_MODULE_H */
