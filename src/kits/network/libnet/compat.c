/* compat.c */

/* These routines are included in libnet simply because R5 expects them
 * to be there. They should mostly be no-ops...
 * Some of them are hacks! We must fix this!!!
 */

#include <fcntl.h>
#include <unistd.h>
#include <OS.h>
#include <iovec.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <net_settings.h>
#include <netdb.h>

#include <TLS.h>

static int32 h_errno_tls = -1;

void _init(void);
void _fini(void);
void initialize_before(void);
void terminate_after(void);

const char * hstrerror(int error);

int _socket_interrupt(void);
int _netconfig_find(void);


_EXPORT void initialize_before(void)
{
	h_errno_tls = tls_allocate();
}

_EXPORT void terminate_after(void)
{
}


_EXPORT int closesocket(int sock)
{
	return close(sock);
}


_EXPORT const char * hstrerror(int error)
{
	printf("hstrerror() not yet supported.");
	return "hstrerror() not supported yet";
}


_EXPORT void herror(const char *error)
{
	printf("herror() not yet supported.");
}

_EXPORT int * _h_errnop()
{
	return (int *) tls_address(h_errno_tls);
}


_EXPORT int _socket_interrupt(void)
{
	printf("_socket_interrupt\n");
	return 0;
}

_EXPORT int _netconfig_find(void)
{
	printf("_netconfig_find\n");
	return 0;
}


/* XXX: HACK HACK HACK! FIXME! */
/* This is a terrible hack :(
 * TODO: We should really get these settings values by parsing
 * $HOME/config/settings/network file, which will make both R5 and BONE compatible.
 */

_EXPORT char * find_net_setting(net_settings * ncw, const char * heading,
	const char * name, char * value, unsigned nbytes)
{
//	printf("find_net_setting\n");
	
	if (strcmp(heading, "GLOBAL") != 0)
		return NULL;
	
	if (strcmp(name, "HOSTNAME") == 0)
		strncpy(value, "hostname", nbytes);
	else if (strcmp(name, "USERNAME") == 0)
		strncpy(value, "baron\0", nbytes);
	else if (strcmp(name, "PASSWORD") == 0)
		strncpy(value, "password", nbytes);
	else
		return NULL;
	
	return value;
}


_EXPORT status_t set_net_setting(net_settings * ncw, const char * heading,
	const char * name, const char * value)
{
	printf("set_net_setting\n");
	return B_UNSUPPORTED;
}


_EXPORT int gethostname(char * name, size_t length)
{
//	printf("gethostname\n");
	if (find_net_setting(NULL, "GLOBAL", "HOSTNAME", name, length) == NULL)
		return B_ERROR;
	
	return strlen(name);
}


_EXPORT int getusername(char * name, size_t length)
{
//	printf("getusername\n");
	if (find_net_setting(NULL, "GLOBAL", "USERNAME", name, length) == NULL)
		return B_ERROR;
	
	return strlen(name);
}


_EXPORT int getpassword(char * pwd, size_t length)
{
//	printf("getpassword\n");
	if (find_net_setting(NULL, "GLOBAL", "PASSWORD", pwd, length) == NULL)
		return B_ERROR;
	
	return strlen(pwd);
}


struct utsname {
	char sysname[32];
	char nodename[32];
	char release[32];
	char version[32];
	char machine[32];
};
int uname(struct utsname *name);
_EXPORT int uname(struct utsname *name)
{
	if(!name)
		return B_ERROR;
	
	strcpy(name->sysname, "BeOS");
	strcpy(name->nodename, "trantor");
	strcpy(name->release, "5.0");
	strcpy(name->version, "1000009");
	strcpy(name->machine, "BePC");
	return B_OK;
}
