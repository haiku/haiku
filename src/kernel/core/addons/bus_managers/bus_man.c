/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>
#include <kernel.h>
#include <lock.h>
#include <debug.h>
#include <malloc.h>
#include <vfs.h>
#include <bus.h>
#include <string.h>
#include <errno.h>
#include <fd.h>
#include <sys/stat.h>

#include <pci_bus.h>

typedef struct bus {
	struct bus *next;
	const char *path;
} bus;

static bus *bus_list;
static mutex bus_lock;


int
bus_man_init(kernel_args *ka)
{
	mutex_init(&bus_lock, "bus_lock");

	bus_list = NULL;

	return 0;
}


static bus *
find_bus(const char *path)
{
	bus *b;

	for (b = bus_list; b != NULL; b = b->next) {
		if (!strcmp(b->path, path))
			break;
	}
	return b;
}


int
bus_register_bus(const char *path)
{
	bus *b;
	int err = 0;

	dprintf("bus_register_bus: path '%s'\n", path);

	mutex_lock(&bus_lock);

	if (!find_bus(path)) {
		b = (bus *)malloc(sizeof(bus));
		if (b == NULL) {
			err = ENOMEM;
			goto err;
		}

		b->path = malloc(strlen(path)+1);
		if (b->path == NULL) {
			err = ENOMEM;
			free(b);
			goto err;
		}
		strcpy((char *)b->path, path);

		b->next = bus_list;
		bus_list = b;
	} else {
		err = ENODEV;
		goto err;
	}

	err = 0;
err:
	mutex_unlock(&bus_lock);
	return err;
}


static int
bus_find_device_recurse(int *n, char *base_path, int max_path_len, int base_fd, id_list *vendor_ids, id_list *device_ids)
{
	char buffer[sizeof(struct dirent) + SYS_MAX_NAME_LEN + 1];
	struct dirent *dirent = (struct dirent *)buffer;
	int base_path_len = strlen(base_path);
	ssize_t len;
	int err;

	while ((len = sys_read_dir(base_fd, dirent, sizeof(buffer), 1)) > 0) {
		struct stat st;
		int fd;

		// reset the base_path to the original string passed in
		base_path[base_path_len] = '\0';
		dirent->d_name[dirent->d_reclen] = '\0';
		strlcat(base_path, dirent->d_name, max_path_len);

		err = stat(base_path, &st);
		if (err < 0)
			continue;

		if (S_ISDIR(st.st_mode)) {
			fd = sys_open_dir(base_path);
			if (fd < 0)
				continue;

			strlcat(base_path, "/", max_path_len);
			err = bus_find_device_recurse(n, base_path, max_path_len, fd, vendor_ids, device_ids);
			sys_close(fd);
			if (err >= 0)
				return err;
			continue;
		} else if (S_ISREG(st.st_mode)) {
			// we opened the device
			// XXX assumes PCI
			struct pci_cfg cfg;
			uint32 i, j;

			fd = sys_open(base_path, 0);
			if (fd < 0)
				continue;

			err = sys_ioctl(fd, PCI_GET_CFG, &cfg, sizeof(struct pci_cfg));
			if (err >= 0) {
				// see if the vendor & device id matches
				for (i = 0; i < vendor_ids->num_ids; i++) {
					if (cfg.vendor_id == vendor_ids->id[i]) {
						for (j = 0; j < device_ids->num_ids; j++) {
							if (cfg.device_id == device_ids->id[j]) {
								// found it
								(*n)--;
								if (*n <= 0)
									return fd;
							}
						}
					}
				}
			}
			sys_close(fd);
		}
	}
	return ENODEV;
}


int
bus_find_device(int n, id_list *vendor_ids, id_list *device_ids, device *dev)
{
	int base_fd;
	int fd;
	char path[256];
	bus *b;
	int err = -1;

	for (b = bus_list; b != NULL && err < 0; b = b->next) {
		base_fd = sys_open_dir(b->path);
		if (base_fd < 0)
			continue;

		strlcpy(path, b->path, sizeof(path));
		strlcat(path, "/", sizeof(path));
		fd = bus_find_device_recurse(&n, path, sizeof(path), base_fd, vendor_ids, device_ids);
		if (fd >= 0) {
			// we have a device!
			// XXX assumes pci
			struct pci_cfg cfg;
			int i;

			err = sys_ioctl(fd, PCI_GET_CFG, &cfg, sizeof(struct pci_cfg));
			if (err >= 0) {
				// copy the relevant data from the pci config to the more generic config
				memset(dev, 0, sizeof(device));
				dev->vendor_id = cfg.vendor_id;
				dev->device_id = cfg.device_id;
				dev->irq = cfg.irq;
				for(i=0; i<6; i++) {
					dev->base[i] = cfg.base[i];
					dev->size[i] = cfg.size[i];
				}
				strcpy(dev->dev_path, path);
			}
			sys_close(fd);
		}
		sys_close(base_fd);
	}

	return err;
}

