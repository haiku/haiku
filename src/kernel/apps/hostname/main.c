/* naive implementation of hostname
 *
 * This mainly serves as a testbed for sysctl, part of the kernel
 * just added.
 */

#include <syscalls.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sysctl.h>

#define MAXHOSTNAMELEN  256 

int main(int argc, char *argv[])
{
	char buffer[MAXHOSTNAMELEN];
	size_t buflen = MAXHOSTNAMELEN;
	int mib[2];
	int rc = -1;
	char *newname = NULL;
	size_t newnamelen = 0;
	
	if (argc >= 2) {
		newname = argv[1];
		newnamelen = strlen(newname);
	}
	
	buffer[0] = '\0';
	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	
	rc = sysctl(mib, 2, &buffer, &buflen, newname, newnamelen);
	
	if (rc == 0 && !newname) {
		printf("%s\n", buffer);
	}
	
	return 0;
}


