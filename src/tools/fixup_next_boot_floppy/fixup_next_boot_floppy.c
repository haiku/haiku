/*
 * Copyright 2020, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <disklabel.h>

/* for now we actually write all of the disk label here. */

//#define DL_SIZE (sizeof(disklabel))
#define DL_SIZE (offsetof(struct disk_label, dl_un.DL_v3_checksum) + sizeof(uint16_t))
#define SUM_CNT (offsetof(struct disk_label, dl_un.DL_v3_checksum) / sizeof(uint16_t))

// could not get ByteOrder.h to include
#define H2B32 htonl
#define H2B16 htons
#define B2H16 ntohs

// see https://unix.superglobalmegacorp.com/darwin01/newsrc/machdep/i386/checksum_16.c.html
static uint16_t checksum_16(uint16_t *p, int count)
{
	uint32_t sum = 0;
	for (;count--;)
		sum += B2H16(*p++);
	// sum both shorts
	sum = (sum & 0x0ffff) + (sum >> 16);
	if (sum > 65535)
		sum -= 65535;
	return sum;
}

int main(int argc, char **argv)
{
	int fd;
	unsigned int i;
	uint16_t sum;
	struct disk_label disklabel = {
		H2B32(DL_V3),
		H2B32(0),
		H2B32(0),
		"NextBoot",//"HaikuBoot",
		H2B32(0),
		H2B32(0xa991637a), //H2B32(0x4841494b), // dl_tag: 'HAIK'
		"Sony MPX-111N 2880-512", // same as NS image, not sure it matters
		"removable_rw_floppy",
		H2B32(1024), // !! 1024 bytes / sector !!
		H2B32(2),
		H2B32(9),
		H2B32(80),
		H2B32(300),
		H2B16(96),
		H2B16(0),
		H2B16(0),
		H2B16(0),
		H2B16(0),
		H2B16(0),
		H2B32(0x20),H2B32(0xffffffff),	// boot block locations XXX: move it closer to start?
		"fdmach", //"haiku_loader",
		"silly", //"schredder",
		'a', 'b',
		// partitions
		{
			// Nextstep uses this:
			{ H2B32(0), H2B32(0x540), H2B16(0x2000), H2B16(0x400), 't', H2B16(0x20), H2B16(0x800), 0, 1, "", 1, "4.3BSD"},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""},
			{ H2B32(-1), H2B32(-1), H2B16(-1), H2B16(-1), 0, H2B16(-1), H2B16(-1), -1, 0, "", 0, ""}
		}
	};
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		return 1;
	}
	//XXX: implement support simple checksum of existing label?
	/*
	if (read(fd, bootblock, DL_SIZE) < DL_SIZE) {
		perror("read");
		return 1;
	}
	*/
	sum = checksum_16((uint16_t *)&disklabel, SUM_CNT);
	fprintf(stderr, "checksum: 0x%04x\n", sum);
	disklabel.dl_un.DL_v3_checksum = H2B16(sum);
	lseek(fd, 0LL, SEEK_SET);
	write(fd, &disklabel, DL_SIZE);
	return 0;
}
