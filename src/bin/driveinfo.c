#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <Drivers.h>
#include <StorageDefs.h>

static void dump_dev_size(int dev)
{
	size_t sz;
	if (ioctl(dev, B_GET_DEVICE_SIZE, &sz, sizeof(sz)) < 0) {
		perror("ioctl(B_GET_DEVICE_SIZE)");
		return;
	}
	printf("size: %ld bytes\n", sz);
}

static void dump_bios_id(int dev)
{
	uint8 id;
	if (ioctl(dev, B_GET_BIOS_DRIVE_ID, &id, sizeof(id)) < 0) {
		perror("ioctl(B_GET_BIOS_DRIVE_ID)");
		return;
	}
	printf("bios id: %d, 0x%x\n", id, id);
}

static void dump_media_status(int dev)
{
	uint32 st;
	if (ioctl(dev, B_GET_MEDIA_STATUS, &st, sizeof(st)) < 0) {
		perror("ioctl(B_GET_MEDIA_STATUS)");
		return;
	}
	printf("media status: %s\n", strerror(st));
}

static const char *device_type(uint32 type)
{
	if (type == B_DISK) return "disk";
	if (type == B_TAPE) return "tape";
	if (type == B_PRINTER) return "printer";
	if (type == B_CPU) return "cpu";
	if (type == B_WORM) return "worm";
	if (type == B_CD) return "cd";
	if (type == B_SCANNER) return "scanner";
	if (type == B_OPTICAL) return "optical";
	if (type == B_JUKEBOX) return "jukebox";
	if (type == B_NETWORK) return "network";
	return "<unknown>";
}

static void dump_geom(int dev, bool bios)
{
	device_geometry geom;
	
	if (ioctl(dev, bios?B_GET_BIOS_GEOMETRY:B_GET_GEOMETRY, &geom, sizeof(geom)) < 0) {
		perror(bios ? "ioctl(B_GET_BIOS_GEOMETRY)" : "ioctl(B_GET_GEOMETRY)");
		return;
	}
	printf("%sgeometry:\n", bios?"bios ":"");
	printf("bytes_per_sector:\t%ld\n", geom.bytes_per_sector);
	printf("sectors_per_track:\t%ld\n", geom.sectors_per_track);
	printf("cylinder_count:\t%ld\n", geom.cylinder_count);
	printf("head_count:\t%ld\n", geom.head_count);
	printf("device_type:\t%d, %s\n", geom.device_type, device_type(geom.device_type));
	printf("%sremovable.\n", geom.removable?"":"not ");
	printf("%sread_only.\n", geom.read_only?"":"not ");
	printf("%swrite_once.\n", geom.write_once?"":"not ");
}

static void dump_partition(int dev)
{
	partition_info partition;
	
	if (ioctl(dev, B_GET_PARTITION_INFO, &partition, sizeof(partition)) < 0) {
		perror("ioctl(B_GET_PARTITION_INFO)");
		return;
	}
	printf("partition:\n");
	printf("offset:\t%lld\n", partition.offset);
	printf("size:\t%lld\n", partition.size);
	printf("logical_block_size:\t%ld\n", partition.logical_block_size);
	printf("session:\t%ld\n", partition.session);
	printf("partition:\t%ld\n", partition.partition);
	printf("device:\t%s\n", partition.device);
}

static void dump_misc(int dev)
{
	char path[B_PATH_NAME_LENGTH];
	if (ioctl(dev, B_GET_DRIVER_FOR_DEVICE, path, sizeof(path)) < 0) {
		perror("ioctl(B_GET_DRIVER_FOR_DEVICE)");
	} else {
		printf("driver:\t%s\n", path);
	}
#ifdef __HAIKU__
	if (ioctl(dev, B_GET_PATH_FOR_DEVICE, path, sizeof(path)) < 0) {
		perror("ioctl(B_GET_PATH_FOR_DEVICE)");
	} else {
		printf("device path:\t%s\n", path);
	}
#endif
}

int main(int argc, char **argv)
{
	int dev;
	if (argc < 2) {
		printf("%s device\n", argv[0]);
		return 1;
	}
	dev = open(argv[1], O_RDONLY);
	if (dev < 0) {
		perror("open");
		return 1;
	}
	dump_dev_size(dev);
	puts("");
	dump_bios_id(dev);
	puts("");
	dump_media_status(dev);
	puts("");
	dump_geom(dev, false);
	puts("");
	dump_geom(dev, true);
	puts("");
	dump_partition(dev);
	puts("");
	dump_misc(dev);
	puts("");
	return 0;
}
