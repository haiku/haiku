// netfs_ioctl.h

#ifndef NET_FS_IOCTL_H
#define NET_FS_IOCTL_H

// our ioctl commands
enum {
	NET_FS_IOCTL_ADD_SERVER		= 11000,
	NET_FS_IOCTL_REMOVE_SERVER,
};

// parameter for NET_FS_IOCTL_ADD_SERVER
struct netfs_ioctl_add_server {
	char	serverName[256];
};

// parameter for NET_FS_IOCTL_REMOVE_SERVER
struct netfs_ioctl_remove_server {
	char	serverName[256];
};

#endif	// NET_FS_IOCTL_H
