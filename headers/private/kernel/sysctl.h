/* sysctl.h
 * should really be installed as sys/sysctl.h
 */

#ifndef _SYS_SYSCTL_H
#define _SYS_SYSCTL_H

/* This file contains definitions for the sysctl call.
 * The call uses a heirachy of names for objects that can be
 * examined or modified. The name is expressed as a sequence
 * of integers and works in a similar manner to a filename, ie each
 * component is dependant upon it's place in the heirachy.
 *
 * NB this file defines only the top level and kernel level (_KERN)
 *    identifiers with others being defined in files specific to
 *    the subsystem concerned.
 */

#define CTL_MAXNAME 12  /* This is the largest no. of components we support */

/* Each subsystem defines a list of variables for that subsystem.
 * The name can either be:
 * - a node that has further levels defined below it, or
 * - a leaf in a particular type.
 * Each level of sysctl has a set of name/type pairs used when manipulating
 * that level.
 */
 
struct ctlname {
	char *ctl_name;   /* the subsystem name */
	int   ctl_type;   /* type of name */
};

#define CTLTYPE_NODE     1  /* it's a node */
#define CTLTYPE_INT      2  /* integer */
#define CTLTYPE_STRING   3  /* string */
#define CTLTYPE_QUAD     4  /* 64-bit number */
#define CTLTYPE_STRUCT   5  /* it's a structure */

/* The top level identifiers we support */
#define CTL_UNSPEC       0  /* unused */
#define CTL_KERN         1  /* Kernel level */
#define CTL_VM           2  /* VM */
#define CTL_FS           3  /* Filesystem */
#define CTL_NET          4  /* networking (socket.h) */
#define CTL_HW		 5  /* hardware */

#define CTL_NAMES { \
	{ 0, 0 }, \
	{ "kern", CTLTYPE_NODE }, \
	{ "vm"  , CTLTYPE_NODE }, \
	{ "fs"  , CTLTYPE_NODE }, \
	{ "net" , CTLTYPE_NODE }, \
	{ "hardware", CTL_TYPE_NODE }, \
}

/* Identifiers for use in the CTL_KERN subsystem  */
#define KERN_OSTYPE      1  /* string: system type string */
#define KERN_OSRELEASE   2  /* string: system release */
#define KERN_OSVERSION   3  /* int: system revision */
#define KERN_HOSTNAME    4  /* string: hostname of system */
#define KERN_DOMAINNAME  5  /* string: (YP) domain name */
#define KERN_VERSION	 6  /* string: system version string */

#define CTL_KERN_NAMES { \
	{ 0, 0 }, \
	{ "ostype"     , CTLTYPE_STRING }, \
	{ "osrelease"  , CTLTYPE_STRING }, \
	{ "osversion"  , CTLTYPE_STRING }, \
	{ "hostname"   , CTLTYPE_STRING }, \
	{ "domainname" , CTLTYPE_STRING }, \
	{ "version"    , CTLTYPE_STRING }, \
}

/*
 * CTL_HW identifiers
 */
#define	HW_MACHINE	 1		/* string: machine class */
#define	HW_MODEL	 2		/* string: specific machine model */
#define	HW_NCPU		 3		/* int: number of cpus */
#define	HW_BYTEORDER	 4		/* int: machine byte order */
#define	HW_PHYSMEM	 5		/* int: total memory */
#define	HW_USERMEM	 6		/* int: non-kernel memory */
#define	HW_PAGESIZE	 7		/* int: software page size */
#define	HW_DISKNAMES	 8		/* strings: disk drive names */
#define	HW_DISKSTATS	 9		/* struct: diskstats[] */
#define	HW_DISKCOUNT	10		/* int: number of disks */
#define	HW_MAXID	11		/* number of valid hw ids */

#define	CTL_HW_NAMES { \
	{ 0, 0 }, \
	{ "machine", CTLTYPE_STRING }, \
	{ "model", CTLTYPE_STRING }, \
	{ "ncpu", CTLTYPE_INT }, \
	{ "byteorder", CTLTYPE_INT }, \
	{ "physmem", CTLTYPE_INT }, \
	{ "usermem", CTLTYPE_INT }, \
	{ "pagesize", CTLTYPE_INT }, \
	{ "disknames", CTLTYPE_STRING }, \
	{ "diskstats", CTLTYPE_STRUCT }, \
	{ "diskcount", CTLTYPE_INT }, \
}


#ifdef _KERNEL_MODE
  typedef int (sysctlfn)(int *, uint, void *, size_t *, void *, size_t);

  int kern_sysctl(int *, uint, void *, size_t *, void *, size_t);
  int hw_sysctl(int *, uint, void *, size_t *, void *, size_t);
  int sysctl_int (void *, size_t *, void *, size_t, int *);
  int sysctl_rdint (void *, size_t *, void *, int);
  int sysctl_tstring(void *, size_t *, void *, size_t, char *, int);
  int sysctl__string(void *, size_t *, void *, size_t, char *, int, int);
  int sysctl_rdstring(void *, size_t *, void *, char *);
  int user_sysctl(int *, uint, void *, size_t *, void *, size_t);
#endif

int sysctl(int *, uint, void *, size_t *, void *, size_t);

#endif /* _SYS_SYSCTL_H */

