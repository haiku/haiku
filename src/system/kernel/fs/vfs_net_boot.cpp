/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "vfs_net_boot.h"

#include <dirent.h>
#include <errno.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <DiskDeviceTypes.h>

#include <disk_device_manager/KDiskDevice.h>

#include <KPath.h>


static bool
string_starts_with(const char* string, const char* prefix)
{
	size_t stringLen = strlen(string);
	size_t prefixLen = strlen(prefix);
	return (stringLen >= prefixLen && strncmp(string, prefix, prefixLen) == 0);
}


static bool
is_net_device(KDiskDevice* device)
{
	const char* path = device->Path();
	return (string_starts_with(path, "/dev/disk/virtual/nbd/")
		|| string_starts_with(path, "/dev/disk/virtual/remote_disk/"));
}


static int
compare_partitions_net_devices(const void *_a, const void *_b)
{
	KPartition* a = *(KPartition**)_a;
	KPartition* b = *(KPartition**)_b;

	bool aIsNetDevice = is_net_device(a->Device());
	bool bIsNetDevice = is_net_device(b->Device());

	int compare = (int)aIsNetDevice - (int)bIsNetDevice;
	if (compare != 0)
		return compare;

	return compare_image_boot(_a, _b);
}


class NetStackInitializer {
public:
	NetStackInitializer(uint64 clientMAC, uint32 clientIP, uint32 netMask)
		:
		fSocket(-1),
		fLinkSocket(-1),
		fClientMAC(clientMAC),
		fClientIP(clientIP),
		fNetMask(netMask),
		fFoundInterface(false),
		fConfiguredInterface(false)
	{
	}

	~NetStackInitializer()
	{
		// close control sockets
		if (fSocket >= 0)
			close(fSocket);

		if (fLinkSocket >= 0)
			close(fLinkSocket);
	}

	status_t Init()
	{
		// open a control socket for playing with the stack
		fSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (fSocket < 0) {
			dprintf("NetStackInitializer: Failed to open socket: %s\n",
				strerror(errno));
			return errno;
		}

		// ... and a link level socket
		fLinkSocket = socket(AF_LINK, SOCK_DGRAM, 0);
		if (fLinkSocket < 0) {
			dprintf("NetStackInitializer: Failed to open link level socket:"
				" %s\n", strerror(errno));
			return errno;
		}


		// now iterate through the existing network devices
		KPath path;
		status_t error = path.SetTo("/dev/net");
		if (error != B_OK)
			return error;

		_ScanDevices(path);

		return fConfiguredInterface ? B_OK : B_ERROR;
	}

private:
	void _ScanDevices(KPath& path)
	{
		DIR* dir = opendir(path.Path());
		if (!dir) {
			dprintf("NetStackInitializer: Failed to opendir() \"%s\": %s\n",
				path.Path(), strerror(errno));
			return;
		}

		while (dirent* entry = readdir(dir)) {
			// skip "." and ".."
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			path.Append(entry->d_name);

			struct stat st;
			if (stat(path.Path(), &st) == 0) {
				if (S_ISDIR(st.st_mode))
					_ScanDevices(path);
				else if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
					_ScanDevice(path.Path());
			}

			path.RemoveLeaf();

			if (fFoundInterface)
				break;
		}

		closedir(dir);
	}

	void _ScanDevice(const char* path)
	{
		dprintf("NetStackInitializer: scanning device %s\n", path);

		// check if this interface is already known
		ifreq request;
		if (strlen(path) >= IF_NAMESIZE)
			return;
		strcpy(request.ifr_name, path);

		if (ioctl(fSocket, SIOCGIFINDEX, &request, sizeof(request)) < 0) {
			// not known yet -- add it
			ifaliasreq aliasRequest;
			strcpy(aliasRequest.ifra_name, path);
			aliasRequest.ifra_addr.ss_family = AF_UNSPEC;
			aliasRequest.ifra_addr.ss_len = 2;
			aliasRequest.ifra_broadaddr.ss_family = AF_UNSPEC;
			aliasRequest.ifra_broadaddr.ss_len = 2;
			aliasRequest.ifra_mask.ss_family = AF_UNSPEC;
			aliasRequest.ifra_mask.ss_len = 2;

			if (ioctl(fSocket, SIOCAIFADDR, &aliasRequest,
					sizeof(aliasRequest)) < 0) {
				dprintf("NetStackInitializer: adding interface failed for "
					"device %s: %s\n", path, strerror(errno));
				return;
			}
		}

		// bring the interface up (get flags, add IFF_UP)
		if (ioctl(fSocket, SIOCGIFFLAGS, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: getting flags failed for interface "
				"%s: %s\n", path, strerror(errno));
			return;
		}

		int interfaceFlags = request.ifr_flags;
		if (!(interfaceFlags & IFF_UP)) {
			interfaceFlags |= IFF_UP;
			request.ifr_flags = interfaceFlags;
			if (ioctl(fSocket, SIOCSIFFLAGS, &request, sizeof(request)) < 0) {
				dprintf("NetStackInitializer: failed to bring interface up "
					"%s: %s\n", path, strerror(errno));
				return;
			}
		}

		// get the MAC address
		if (ioctl(fLinkSocket, SIOCGIFADDR, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: Getting MAC addresss failed for "
				"interface %s: %s\n", path, strerror(errno));
			return;
		}

		sockaddr_dl& link = *(sockaddr_dl*)&request.ifr_addr;
		if (link.sdl_type != IFT_ETHER)
			return;

		if (link.sdl_alen == 0)
			return;

		uint8* macBytes = (uint8 *)LLADDR(&link);
		uint64 macAddress = ((uint64)macBytes[0] << 40)
			| ((uint64)macBytes[1] << 32)
			| ((uint64)macBytes[2] << 24)
			| ((uint64)macBytes[3] << 16)
			| ((uint64)macBytes[4] << 8)
			| (uint64)macBytes[5];

		dprintf("NetStackInitializer: found ethernet interface with MAC "
			"address %02x:%02x:%02x:%02x:%02x:%02x; which is%s the one we're "
			"looking for\n", macBytes[0], macBytes[1], macBytes[2], macBytes[3],
			macBytes[4], macBytes[5], (macAddress == fClientMAC ? "" : "n't"));

		if (macAddress != fClientMAC)
			return;

		fFoundInterface = true;

		// configure the interface

		// set IP address
		sockaddr_in& address = *(sockaddr_in*)&request.ifr_addr;
		address.sin_family = AF_INET;
		address.sin_len = sizeof(sockaddr_in);
		address.sin_port = 0;
		address.sin_addr.s_addr = htonl(fClientIP);
		memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));
		if (ioctl(fSocket, SIOCSIFADDR, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: Setting IP addresss failed for "
				"interface %s: %s\n", path, strerror(errno));
			return;
		}

		// set net mask
		address.sin_addr.s_addr = htonl(fNetMask);
		if (ioctl(fSocket, SIOCSIFNETMASK, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: Setting net mask failed for "
				"interface %s: %s\n", path, strerror(errno));
			return;
		}

		// set broadcast address
		address.sin_addr.s_addr = htonl(fClientIP | ~fNetMask);
		if (ioctl(fSocket, SIOCSIFBRDADDR, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: Setting broadcast address failed for "
				"interface %s: %s\n", path, strerror(errno));
			return;
		}

		// set IFF_BROADCAST
		if (!(interfaceFlags & IFF_BROADCAST)) {
			interfaceFlags |= IFF_BROADCAST;
			request.ifr_flags = interfaceFlags;
			if (ioctl(fSocket, SIOCSIFFLAGS, &request, sizeof(request)) < 0) {
				dprintf("NetStackInitializer: failed to set IFF_BROADCAST flag "
					"for interface %s: %s\n", path, strerror(errno));
				return;
			}
		}

		// set default route; remove previous one, if any
		route_entry route;
		memset(&route, 0, sizeof(route_entry));
		route.flags = RTF_STATIC | RTF_DEFAULT;

		request.ifr_route = route;
		ioctl(fSocket, SIOCDELRT, &request, sizeof(request));
		if (ioctl(fSocket, SIOCADDRT, &request, sizeof(request)) < 0) {
			dprintf("NetStackInitializer: Failed to set default route: %s\n",
				strerror(errno));
			return;
		}

		fConfiguredInterface = true;

		dprintf("NetStackInitializer: successfully configured boot network "
			"interface\n");
	}

private:
	int					fSocket;
	int					fLinkSocket;
	uint64				fClientMAC;
	uint32				fClientIP;
	uint32				fNetMask;
	bool				fFoundInterface;
	bool				fConfiguredInterface;
};


// #pragma mark - NetBootMethod


NetBootMethod::NetBootMethod(const KMessage& bootVolume, int32 method)
	: BootMethod(bootVolume, method)
{
}


NetBootMethod::~NetBootMethod()
{
}


status_t
NetBootMethod::Init()
{
	// We need to bring up the net stack.
	status_t status;

	uint64 clientMAC;
	uint32 clientIP;
	uint32 netMask;
	if (fBootVolume.FindInt64("client MAC", (int64*)&clientMAC) != B_OK
		|| fBootVolume.FindInt32("client IP", (int32*)&clientIP) != B_OK) {
		panic("no client MAC or IP address or net mask\n");
		return B_ERROR;
	}

	if (fBootVolume.FindInt32("net mask", (int32*)&netMask) != B_OK) {
		// choose default netmask depending on the class of the address
		if (IN_CLASSA(clientIP)
			|| (clientIP >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET) {
			// class A, or loopback
			netMask = IN_CLASSA_NET;
		} else if (IN_CLASSB(clientIP)) {
			// class B
			netMask = IN_CLASSB_NET;
		} else {
			// class C and rest
			netMask = IN_CLASSC_NET;
		}
	}

	NetStackInitializer initializer(clientMAC, clientIP, netMask);
	status = initializer.Init();
	if (status != B_OK)
		return status;

	// TODO: "net root path" should be used for finding the boot device/FS,
	// but ATM neither the remote_disk nor the nbd driver are configurable
	// at this point.
	const char* rootPath = fBootVolume.GetString("net root path", NULL);
	dprintf("NetBootMethod::Init(): net stack initialized; root path is: %s\n",
		rootPath);

	return B_OK;
}


bool
NetBootMethod::IsBootDevice(KDiskDevice* device, bool strict)
{
	// We support only NBD and RemoteDisk at the moment, so we accept any
	// device under /dev/disk/virtual/{nbd,remote_disk}/.
	return is_net_device(device);
}


bool
NetBootMethod::IsBootPartition(KPartition* partition, bool& foundForSure)
{
	// as long as it's BFS, we're fine
	return (partition->ContentType()
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) == 0);
}


void
NetBootMethod::SortPartitions(KPartition** partitions, int32 count)
{
	qsort(partitions, count, sizeof(KPartition*),
		compare_partitions_net_devices);
}
