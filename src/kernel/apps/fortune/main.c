#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscalls.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define FORTUNES "/boot/etc/fortunes"

int
main(void)
{
	int fd;
	int rc;
	char *buf;
	unsigned i;
	unsigned found;
	struct stat stat;

	fd = open(FORTUNES, O_RDONLY, 0);
	if (fd < 0) {
		printf("Couldn't open %s: %s\n", FORTUNES, strerror(errno));
		return -1;
	}

	rc = fstat(fd, &stat);
	if(rc < 0) {
		printf("Cookie monster was here!!!\n");
		sys_exit(1);
	}

	buf= malloc(stat.st_size + 1);
	
	read(fd, buf, stat.st_size);
	buf[stat.st_size]= 0;
	close(fd);

	found= 0;
	for(i= 0; i< stat.st_size; i++) {
		if(strncmp(buf+i, "#@#", 3)== 0) {
			found+= 1;
		}
	}

	found = 1 + (sys_system_time() % found);

	for(i = 0; i < stat.st_size; i++) {
		if(strncmp(buf+i, "#@#", 3)== 0) {
			found-= 1;
		}
		if(found== 0) {
			unsigned j;

			for(j= i+1; j< stat.st_size; j++) {
				if(strncmp(buf+j, "#@#", 3)== 0) {
					buf[j]= 0;
				}
			}

			printf("%s\n", buf+i+3);
			break;
		}
	}

	return 0;
}
