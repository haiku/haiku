// ToDo: do we really need sysctl? And if so, for what?
//	It is not part of the POSIX spec.

#include <kernel.h>
#include <ktypes.h>
#include <string.h>
#include <errno.h>
#include <vm.h>
#include <debug.h>
#include <malloc.h>
#include <sysctl.h>
#include <KernelExport.h>
#include <Errors.h>

/* Not sure where to put this definition yet (sys/param.h?), 
 * so just add it here 
 * XXX - horrible hack!
 */
#ifndef MAXHOSTNAMELEN
#	define MAXHOSTNAMELEN  256             /* max hostname size */
#endif

/* This is the place we store a few of the "global variables" that the OS
 * needs.
 */
char hostname[MAXHOSTNAMELEN] = "openbeos\0";
int  hostnamelen = 9;
char domainname[MAXHOSTNAMELEN] = "rocks.my.world.com\0";
int  domainnamelen = 19;

/* These really don't belong here, but just for the moment until we figure out where
 * these should live...
 */
 
char ostype[] = "OpenBeOS";
char osrelease[] = "0.01";
char osversion[] = "alpha";
char version[] = "0.0.1";
char kernel[] = "DEV";

char machine[] = "Intel";
char model[] =	"MODEL";


typedef int (sysctlfn)(int *, uint, void *, size_t *, void *, size_t);

// currently unused
#if 0
static int
sysctl_int(void *oldp, size_t *oldlenp, void *newp, size_t newlen, int *valp)
{
	if (oldp && *oldlenp < sizeof(int))
		return ENOMEM;
	if (newp && newlen != sizeof(int))
		return EINVAL;

	*oldlenp = sizeof(int);
	if (oldp)
		*(int*)oldp = *valp;
	if (newp)
		*valp = *(int*)newp;

	return 0;
}


static int
sysctl_rdint(void *oldp, size_t *oldlenp, void *newp, int val)
{
	if (oldp && *oldlenp < sizeof(int))
		return ENOMEM;
	if (newp)
		return EPERM;

	*oldlenp = sizeof(int);
	if (oldp)
		*(int*)oldp = val;

	return 0;
}
#endif

static int
sysctl__string(void *oldp, size_t *oldlenp, void *newp, size_t newlen,
	char *str, size_t maxlen, int trunc)
{
	size_t len = strlen(str) + 1;
	int c;

	if (oldp && *oldlenp < len) {
		if (trunc == 0 || *oldlenp == 0)
			return ENOMEM;
	}
	if (newp && newlen >= maxlen)
		return EINVAL;

	if (oldp) {
		if (trunc && *oldlenp < len) {
			/* need to truncate */
			c = str[*oldlenp - 1];
			str[*oldlenp - 1] = '\0';
			memcpy(oldp, str, *oldlenp);
			str[*oldlenp - 1] = c;
		} else {
			/* just copy */
			*oldlenp = len;
			memcpy(oldp, str, len);
		}
	}
	if (newp) {
		memcpy(str, newp, newlen);
		str[newlen] = 0;
	}
	return 0;
}


/** Copy string, truncating if required */

static int
sysctl_tstring(void *oldp, size_t *oldlenp, void *newp, size_t newlen, char *str, int maxlen)
{
	return sysctl__string(oldp, oldlenp, newp, newlen, str, maxlen, 1);
}


static int
sysctl_rdstring(void *oldp, size_t *oldlenp, void *newp, char *str)
{
	size_t len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return ENOMEM;
	if (newp)
		return EPERM;
	*oldlenp = len;
	if (oldp)
		memcpy(oldp, str, len);
	return 0;
}


static int
sys_sysctl(int *name, uint namelen, void *oldp, size_t *oldlenp,
	void *newp, size_t newlen)
{
	int error = 0;

	switch (name[0]) {
		case KERN_OSTYPE:
			return sysctl_rdstring(oldp, oldlenp, newp, ostype);
		case KERN_OSRELEASE:
			return sysctl_rdstring(oldp, oldlenp, newp, osrelease);
		case KERN_OSVERSION:
			return sysctl_rdstring(oldp, oldlenp, newp, osversion);
		case KERN_HOSTNAME:
			error = sysctl_tstring(oldp, oldlenp, newp, newlen, 
			                       hostname, sizeof(hostname));
			if (newp && !error)
				hostnamelen = newlen;
			return (error);
		case KERN_DOMAINNAME:
			error = sysctl_tstring(oldp, oldlenp, newp, newlen, 
			                       domainname, sizeof(domainname));
			if (newp && !error)
				domainnamelen = newlen;
			return (error);
		case KERN_VERSION:
			return sysctl_rdstring(oldp, oldlenp, newp, kernel);
		default:
			return EOPNOTSUPP;
	}
	/* If we get here we're in trouble... */
}


static int
hw_sysctl(int *name, uint namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	switch (name[0]) {
		case HW_MACHINE:
			return  sysctl_rdstring(oldp, oldlenp, newp, machine);
		case HW_MODEL:
			return  sysctl_rdstring(oldp, oldlenp, newp, model);
		default:
			return  EOPNOTSUPP;
	}
	/* If we get here we're in trouble... */
}


int
sysctl(int *name, uint namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	sysctlfn *fn = NULL;
	int error = 0;

	switch (name[0]) {
		case CTL_KERN:
			fn = sys_sysctl;
			break;
		case CTL_HW:
			fn = hw_sysctl;
			break;
		default:
			dprintf("sysctl: no suppport added yet for %d\n", name[0]);
			return EOPNOTSUPP;
	}
	error = (fn)(name + 1, namelen - 1, oldp, oldlenp, newp, newlen);
	
	return B_NO_ERROR;

}


int
user_sysctl(int *name, uint namelen, void *oldp, size_t *oldlenp,
           void *newp, size_t newlen)
{
	void *a1 = NULL, *a2 = NULL;
	int *nam = NULL, rc;
	size_t olen = *oldlenp, *ov=NULL;

	if (namelen < 2 || namelen > CTL_MAXNAME)
		return EINVAL;

	if (name && namelen > 0) {
		nam = (int *)malloc(namelen * sizeof(int));
		if (!nam)
			return ENOMEM;
		user_memcpy(nam, name, namelen * sizeof(int));
	}
	if (oldp && oldlenp) {
		a1 = malloc(olen);
		user_memcpy(a1, oldp, olen);
	}
	if (oldlenp) {
		ov = (size_t*)malloc(sizeof(oldlenp));
		user_memcpy(ov, oldlenp, sizeof(oldlenp));
	}
	if (newp && newlen > 0) {
		a2 = malloc(newlen);
		user_memcpy(a2, newp, newlen);
	}
	
	rc = sysctl(nam, namelen, a1, ov, a2, newlen);

	if (nam)
		free(nam);

	if (rc != 0)
		goto bailout;

	/* We don't need to copy back the name settings as they won't have changed. */
	if (oldp && *ov > 0) {
		memcpy(oldp, a1, *ov);
		memcpy(oldlenp, ov, sizeof(oldlenp));
	}
	if (newp && newlen > 0) {
		/* We passed namelen thru so don't worry about updating it here... */
		memcpy(newp, a2, newlen);
	}

bailout:
	free(a1);
	free(a2);
	free(ov);

	return rc;
}

