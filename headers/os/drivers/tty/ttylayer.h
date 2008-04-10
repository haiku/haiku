/*
 *	BeOS R5 compatible TTY module API
 *
 *	Copyright 2008, Haiku Inc. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#ifndef _TTY_TTYLAYER_H
#define _TTY_TTYLAYER_H

#include <module.h>
#include <termios.h>

struct ddrover {
	
};

struct ddomain {
	
};

typedef bool (*tty_service_func)(struct tty *, struct ddrover *, uint );

struct tty {
	uint	nopen;
	uint	flags;
	struct ddomain	dd;
	struct ddomain	ddi;	// used in interrupt context
	pid_t	pgid;
	struct termios	t;
	uint	iactivity;
	bool	ibusy;
	struct str istr;
	struct str rstr;
	struct str ostr;
	struct winsize	wsize;
	tty_service_func	service;
	struct sel	*sel;
	struct tty	*ttysel;
};

struct ttyfile {
	struct tty	*tty;
	uint		flags;
	bigtime_t	vtime;
};


typedef struct tty_module_info tty_module_info;

struct tty_module_info {
	// not a real bus manager... no rescan() !
	module_info	mi;
	status_t	(*ttyopen)(struct ttyfile *, struct ddrover *, tty_service_func);
	status_t	(*ttyclose)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyfree)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyread)(struct ttyfile *, struct ddrover *, char *, size_t *);
	status_t	(*ttywrite)(struct ttyfile *, struct ddrover *, const char *, size_t *);
	status_t	(*ttycontrol)(struct ttyfile *, struct ddrover *, ulong, void *, size_t);
#if 0 /* Dano! */
	status_t	(*ttyselect)(struct ttyfile *, struct ddrover *, uint8, uint32, selectsync *);
	status_t	(*ttydeselect)(struct ttyfile *, struct ddrover *, uint8, selectsync *);
#endif
	void	(*ttyinit)(struct tty *, bool);
	void	(*ttyilock)(struct tty *, struct ddrover *, bool );
	void	(*ttyhwsignal)(struct tty *, struct ddrover *, int, bool);
	int	(*ttyin)(struct tty *, struct ddrover *, int);
	int	(*ttyout)(struct tty *, struct ddrover *);
	struct ddrover	*(*ddrstart)(struct ddrover *);
	void	(*ddrdone)(struct ddrover *);
	void	(*ddracquire)(struct ddrover *, struct ddomain *);
};

#define B_TTY_MODULE_NAME		"bus_managers/tty/v1"

#endif /* _TTY_TTYLAYER_H */
