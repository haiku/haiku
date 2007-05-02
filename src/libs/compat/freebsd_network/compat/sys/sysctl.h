#ifndef _FBSD_COMPAT_SYS_SYSCTL_H_
#define _FBSD_COMPAT_SYS_SYSCTL_H_

#ifdef _KERNEL

struct sysctl_req {
	void *newptr;
};

#define SYSCTL_HANDLER_ARGS void *oidp, void *arg1, void *arg2, struct sysctl_req *req

#define OID_AUTO	(-1)

#define CTLTYPE			0xf
#define CTLTYPE_NONE	1
#define CTLTYPE_INT		2
#define CTLTYPE_STRING	3

#define CTLFLAG_RD		0x80000000
#define CTLFLAG_WR		0x40000000
#define CTLFLAG_RW		(CTLFLAG_RD | CTLFLAG_WR)

static void SYSCTL_ADD_PROC(void *a, void *b, int c, const char *d, int e,
	void *f, int g, int (*h)(SYSCTL_HANDLER_ARGS), const char *i, const char *j)
{
}

#define SYSCTL_ADD_INT(...)
#define SYSCTL_CHILDREN(...)	NULL

#define device_get_sysctl_ctx(dev)	NULL

static inline int
sysctl_handle_int(SYSCTL_HANDLER_ARGS)
{
	return 0;
}

#endif

#endif
