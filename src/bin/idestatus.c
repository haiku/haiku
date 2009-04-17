#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <SupportDefs.h>
#include <BeBuild.h>

/* why isn't that documented ? */
#define IDE_GET_STATUS 0x2711

typedef struct {
	uint8 dummy;
	uint8 dma_status;
	uint8 pio_mode;
	uint8 dma_mode;
} _PACKED ide_status_t;

char *dma_status_strings[] = {
	"drive does not support dma",
	"dma enabled",
	"dma disabled by user config",
	"dma disabled by safe mode",
	"ide controller does not support dma",
	"driver disabled dma due to inproper drive configuration",
	"dma disabled after dma failure"
};

char *dma_mode_strings[] = {
	"Multiword DMA Mode 0 - 4.2 MB/s",
	"Multiword DMA Mode 1 - 13.3 MB/s",
	"Multiword DMA Mode 2 - 16.7 MB/s",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Invalid",
	"Ultra DMA Mode 0 - 16.7 MB/s",
	"Ultra DMA Mode 1 - 25 MB/s",
	"Ultra DMA Mode 2 - 33.3 MB/s",
	"Ultra DMA Mode 3 - 44.4 MB/s",
	"Ultra DMA Mode 4 - 66.7 MB/s",
	"Ultra DMA Mode 5 - 100 MB/s",
	"Ultra DMA Mode 6 - 133 MB/s"
};

int main(int argc, char **argv)
{
	int fd;
	ide_status_t st = {0x01, 0x00, 0x00, 0x80}; /* God knows... */
	if (argc < 2) {
		fprintf(stderr, "use: %s devicename\n", argv[0]);
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "could not open %s, %s\n", argv[1], strerror(errno));
		return 2;
	}
	if (ioctl(fd, IDE_GET_STATUS, &st, sizeof(ide_status_t *)) < 0) {
		fprintf(stderr, "could not get ide status for %s\n", argv[1]);
		return 3;
	}
	if (st.dma_status > 6)
		printf("dma_status bad\n");
	else
		printf("dma_status: %s\n", dma_status_strings[st.dma_status]);
	printf("pio mode: %d\n", st.pio_mode);
	printf("dma mode: %s\n", dma_mode_strings[st.dma_mode]);
	close(fd);
	return 0;
}
