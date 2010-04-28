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


// XXX: should go away or be opacised
struct str {
	uchar	*buffer;
	uint	bufsize, count, tail;
	bool	allocated;
};

struct ddrover {
	
};

struct ddomain {
	struct ddrover	*r;
	bool	bg;
	bool	locked;
};
#define ddbackground(d) ((d)->bg = true)


typedef bool (*tty_service_func)(struct tty *tty, struct ddrover *rover, uint op);

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
#define	TTYENABLE		0
#define	TTYDISABLE		1
#define	TTYSETMODES		2
#define	TTYOSTART		3
#define	TTYOSYNC		4
#define	TTYISTOP		5
#define	TTYIRESUME		6
#define	TTYSETBREAK		7
#define	TTYCLRBREAK		8
#define	TTYSETDTR		9
#define	TTYCLRDTR		10
#define	TTYGETSIGNALS	11

typedef struct tty_module_info tty_module_info;

// this version is compatible with BeOS R5
struct tty_module_info_r5 {
	// not a real bus manager... no rescan() !
	module_info	mi;
	status_t	(*ttyopen)(struct ttyfile *, struct ddrover *, tty_service_func);
	status_t	(*ttyclose)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyfree)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyread)(struct ttyfile *, struct ddrover *, char *, size_t *);
	status_t	(*ttywrite)(struct ttyfile *, struct ddrover *, const char *, size_t *);
	status_t	(*ttycontrol)(struct ttyfile *, struct ddrover *, ulong, void *, size_t);
	void	(*ttyinit)(struct tty *, bool);
	void	(*ttyilock)(struct tty *, struct ddrover *, bool );
	void	(*ttyhwsignal)(struct tty *, struct ddrover *, int, bool);
	int	(*ttyin)(struct tty *, struct ddrover *, int);
	int	(*ttyout)(struct tty *, struct ddrover *);
	struct ddrover	*(*ddrstart)(struct ddrover *);
	void	(*ddrdone)(struct ddrover *);
	void	(*ddacquire)(struct ddrover *, struct ddomain *);
};

// BeOS R5.1d0 has a different module with the same version...
// we exort this module as version 1.1 to allow using select from drivers
struct tty_module_info_dano {
	// not a real bus manager... no rescan() !
	module_info	mi;
	status_t	(*ttyopen)(struct ttyfile *, struct ddrover *, tty_service_func);
	status_t	(*ttyclose)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyfree)(struct ttyfile *, struct ddrover *);
	status_t	(*ttyread)(struct ttyfile *, struct ddrover *, char *, size_t *);
	status_t	(*ttywrite)(struct ttyfile *, struct ddrover *, const char *, size_t *);
	status_t	(*ttycontrol)(struct ttyfile *, struct ddrover *, ulong, void *, size_t);
	status_t	(*ttyselect)(struct ttyfile *, struct ddrover *, uint8, uint32, selectsync *);
	status_t	(*ttydeselect)(struct ttyfile *, struct ddrover *, uint8, selectsync *);

	void	(*ttyinit)(struct tty *, bool);
	void	(*ttyilock)(struct tty *, struct ddrover *, bool );
	void	(*ttyhwsignal)(struct tty *, struct ddrover *, int, bool);
	int	(*ttyin)(struct tty *, struct ddrover *, int);
	int	(*ttyout)(struct tty *, struct ddrover *);
	struct ddrover	*(*ddrstart)(struct ddrover *);
	void	(*ddrdone)(struct ddrover *);
	void	(*ddacquire)(struct ddrover *, struct ddomain *);
};

#define B_TTY_MODULE_NAME_R5		"bus_managers/tty/v1"
#define B_TTY_MODULE_NAME_DANO		"bus_managers/tty/v1.1"

#define B_TTY_MODULE_NAME			B_TTY_MODULE_NAME_DANO
#define tty_module_info				tty_module_info_dano

#endif /* _TTY_TTYLAYER_H */
