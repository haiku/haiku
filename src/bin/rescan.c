/*
 * Copyright 2002, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 2002, François Revol
// technical reference:
// http://bedriven.be-in.org/document/280-serial_port_driver.html
// 2004: bedriven is down now, mirror at:
// http://web.archive.org/web/20040220055400/http://bedriven.be-in.org/document/280-serial_port_driver.html

int main(int argc, char **argv)
{
	char *default_scan[] = { "scsi_dsk", "scsi_cd", "ata", "atapi" }; // not really sure here...
	char *default_scan_names[] = { "scsi disks", "scsi cdroms", "ide ata", "ide atapi" };
	char **scan = default_scan;
	char **scan_names = default_scan_names;
	int scan_count = 4;
	int scan_index = 0;
	int fd_dev;

	if (argc == 2 && !strcmp(argv[1], "--help")) {
		printf("usage: rescan [driver]\n");
		return 0;
	}
	if (argc > 1) {
		scan = scan_names = argv;
		scan_count = argc;
		scan_index++; // not argv[0]
	}
	for (; scan_index < scan_count; scan_index++) {
		printf("scanning %s...\n", scan_names[scan_index]);
		fd_dev = open("/dev", O_WRONLY);
		write(fd_dev, scan[scan_index], strlen(scan[scan_index]));
		close(fd_dev);
	}
	return 0;
}

