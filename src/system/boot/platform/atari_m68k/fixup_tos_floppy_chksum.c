#include <stdio.h>
#include <OS.h>
#include <ByteOrder.h>


uint8 sector[512];

int main(int argc, char **argv)
{
	int fd, i;
	uint16 sum;
	uint16 *p = (uint16 *)sector;
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		return 1;
	}
	if (read(fd, sector, 512-2) < 512) {
		perror("read");
		return 1;
	}
	for (sum = 0, i = 0; i < (512-2)/2; i++) {
		sum += B_LENDIAN_TO_HOST_INT16(p[i]);
	}
	p[(512-2)/2] = B_HOST_TO_BENDIAN_INT16(0x1234 - sum);
	//lseek(fd, 0LL, SEEK_SET);
	write(fd, &sector[512-2], 2);
	return 0;
}
