/* compat.c */

/* These routines are included in libnet simply because R5 expects them
 * to be there. They should mostly be no-ops...
 */

#include <fcntl.h>
#include <unistd.h>
#include <kernel/OS.h>
#include <iovec.h>
#include <stdio.h>
#include <string.h>

#include <TLS.h>

static int32 h_errno_tls;

/* These should probably be moved to a seperate file as they
 * are unique to the library as a whole...
 */

void _init()
{
	h_errno_tls = tls_allocate();
}

void _fini()
{
}

struct net_settings {
	int dummy;
};

_EXPORT int closesocket(int sock)
{
	return close(sock);
}


_EXPORT const char * hstrerror(int error)
{
	printf("hstrerror() not yet supported.");
	return "hstrerror() not supported yet";
}


_EXPORT int herror()
{
	printf("herror() not yet supported.");
	return 0;
}

_EXPORT int *_h_errnop()
{
	return (int *)tls_address(h_errno_tls);
}


_EXPORT int _socket_interrupt()
{
	printf("_socket_interrupt\n");
	return 0;
}

_EXPORT int _netconfig_find()
{
	printf("_netconfig_find\n");
	return 0;
}

/* This is a terrible hack :(
 * We should really get these settings values by parsing $HOME/config/settings/network file, which
 * will make both R5 and BONE compatible
 */

_EXPORT char * find_net_setting(struct net_settings * ncw, const char * heading, const char * name, char * value, unsigned nbytes)
{
	  
	printf("find_net_setting\n");
	
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


_EXPORT status_t set_net_setting(struct net_settings * ncw, const char * heading, const char * name, const char * value)
{
	printf("set_net_setting\n");
	return B_UNSUPPORTED;
}


_EXPORT int gethostname(char * name, size_t length)
{
	printf("gethostname\n");
	if (find_net_setting(NULL, "GLOBAL", "HOSTNAME", name, length) == NULL)
		return B_ERROR;
	
	return strlen(name);
}


_EXPORT int getusername(char * name, uint length)
{
	printf("getusername\n");
	if (find_net_setting(NULL, "GLOBAL", "USERNAME", name, length) == NULL)
		return B_ERROR;
	
	return strlen(name);
}


_EXPORT int getpassword(char * pwd, uint length)
{
	printf("getpassword\n");
	if (find_net_setting(NULL, "GLOBAL", "PASSWORD", pwd, length) == NULL)
		return B_ERROR;
	
	return strlen(pwd);
}


