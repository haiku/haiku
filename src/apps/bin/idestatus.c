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
	printf("dma mode: 0x%02x\n", st.dma_mode);
	close(fd);
	return 0;
}
