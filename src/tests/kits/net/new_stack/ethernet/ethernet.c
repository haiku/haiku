/* ethernet.c - ethernet devices interface module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/dirent.h>
#include <sys/stat.h>

#include <Drivers.h>

#include "net_stack.h"
#include "net_interface.h"

enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,
	ETHER_INIT,
	ETHER_NONBLOCK,
	ETHER_ADDMULTI,
	ETHER_REMMULTI,
	ETHER_SETPROMISC,
	ETHER_GETFRAMESIZE
};

typedef struct ethernet_interface {
	ifnet_t ifnet;
	int 	fd;
	int		mtu;
} ethernet_interface;

#define	ETHER_ADDR_LEN	6	/* Ethernet address length		*/
#define ETHER_TYPE_LEN	2	/* Ethernet type field length		*/
#define ETHER_CRC_LEN	4	/* Ethernet CRC lenght			*/
#define ETHER_HDR_LEN	((ETHER_ADDR_LEN * 2) + ETHER_TYPE_LEN)
#define ETHER_MIN_LEN	64	/* Minimum frame length, CRC included	*/
#define ETHER_MAX_LEN	1518	/* Maximum frame length, CRC included	*/

#define	ETHERMTU	(ETHER_MAX_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)
#define	ETHERMIN	(ETHER_MIN_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)

typedef struct        ether_addr { 
        uint8 byte[ETHER_ADDR_LEN]; 
} ether_addr_t; 

	
status_t	lookup_devices(char *root, void *cookie);
status_t 	std_ops(int32 op, ...);

struct net_interface_module_info nimi;

static struct net_stack_module_info *g_stack = NULL;


// -------------------
status_t init(void * params)
{
	return lookup_devices("/dev/net", params);
}

status_t uninit(ifnet_t *iface)
{
	ethernet_interface *ei = (ethernet_interface *) iface;
	
	printf("ethernet: uniniting %s interface\n", iface->if_name);
	
	free(iface->if_name);
	close(ei->fd);
	
	return B_OK;
}


status_t up(ifnet_t *iface)
{
	return B_ERROR;
}


status_t down(ifnet_t *iface)
{
	return B_ERROR;
}


status_t send(ifnet_t *iface, net_data *data)
{
	if (!data)
		return B_ERROR;

		
	return B_OK;
}


status_t receive(ifnet_t *iface, net_data **data)
{
	ethernet_interface *ei = (ethernet_interface *) iface;
	net_data *nd;
	void *frame;
	size_t len;
	status_t status;
	
	len = iface->if_mtu;
	frame = malloc(len);
	if (!frame)
		return B_NO_MEMORY;

	nd = g_stack->new_data();
	if (!nd) {
		free(frame);
		return B_NO_MEMORY;
	};
		
	status = read(ei->fd, frame, len);
	if (status >= B_OK) {
		g_stack->append_data(nd, frame, status, free);
		*data = nd;
		return status;
	};
	
	free(frame);
	g_stack->delete_data(nd, false);
	return status;
}
	
status_t control(ifnet_t *iface, int opcode, void *arg) { return -1; }
	
status_t get_hardware_address(ifnet_t *iface, net_interface_hwaddr *hwaddr) { return -1; }
status_t get_multicast_addresses(ifnet_t *iface, net_interface_hwaddr **hwaddr, int nb_addresses) { return -1; }
status_t set_multicast_addresses(ifnet_t *iface, net_interface_hwaddr *hwaddr, int nb_addresses) { return -1; }
	
status_t set_mtu(ifnet_t *iface, uint32 mtu)  { return -1; }
status_t set_promiscuous(ifnet_t *iface, bool enable) { return -1; }
status_t set_media(ifnet_t *iface, uint32 media)  { return -1; }

// #pragma mark -

status_t lookup_devices(char *root, void *cookie)
{
	DIR *dir;
	struct dirent *de;
	struct stat st;
	char path[1024];
	status_t status;
	int fd;

	dir = opendir(root);
	if (!dir) {
		printf("Couldn't open the directory %s\n", root);
		return B_ERROR;
	};

	status = B_OK;

	while ((de = readdir(dir)) != NULL) {
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;	// skip pseudo-directories
		
		sprintf(path, "%s/%s", root, de->d_name);
		
		if (stat(path, &st) < 0) {
			printf("ethernet: Can't stat(%s) entry! Skipping...\n", path);
			continue;
		};
		
		if (S_ISDIR(st.st_mode))
			status = lookup_devices(path, cookie);
		else if (S_ISCHR(st.st_mode)) {	// char device = driver!
			ifnet_t *iface;
			ethernet_interface *ei;
			int frame_size;
			ether_addr_t mac_address;
	
			if (strcmp(de->d_name, "stack") == 0 ||	// OBOS stack driver
				strcmp(de->d_name, "api") == 0)		// BONE stack driver
				continue;	// skip pseudo-entries
		
			fd = open(path, O_RDWR);
			if (fd < B_OK) {
				printf("ethernet: Unable to open(%s) -> %d [%s]. Skipping...\n", path,
					fd, strerror(fd));
				status = fd;
				continue;
			};
			
			// Init the network card
			status = ioctl(fd, ETHER_INIT, NULL, 0);
			if (status < B_OK) {
				printf("ethernet: Failed to init %s: %ld [%s]. Skipping...\n", path,
					status, strerror(status));
				close(fd);
				continue;
			};
			
			// get the MAC address...
			status = ioctl(fd, ETHER_GETADDR, &mac_address, 6);
			if (status < B_OK) {
				printf("ethernet: Failed to get %s MAC address: %ld [%s]. Skipping...\n",
					path, status, strerror(status));
				close(fd);
				continue;
			};
			
			
			// Try to determine the MTU to use
			status = ioctl(fd, ETHER_GETFRAMESIZE, &frame_size, sizeof(frame_size));
			if (status < B_OK) {
				frame_size = ETHERMTU;
				printf("ethernet: %s device don't support IF_GETFRAMESIZE; defaulting to %d\n",
				       path, frame_size);
			};

			printf("ethernet: interface '%s':\n"
						"\tMAC address: %02x:%02x:%02x:%02x:%02x:%02x\n"
						"\tMTU:         %d bytes\n",
					path,
					mac_address.byte[0], mac_address.byte[1],
					mac_address.byte[2], mac_address.byte[3],
					mac_address.byte[4], mac_address.byte[5], frame_size);
			


			iface = (ifnet_t *) malloc(sizeof(*iface));
			if (!iface)
				break;		

			ei = (ethernet_interface *) iface;

			iface->if_name 	= strdup(path);
			iface->if_flags = (IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST);
			iface->if_type 	= 0x20;
			iface->if_mtu 	= frame_size;
			
			iface->module = &nimi;
			ei->fd = fd;
			
			status = g_stack->register_interface(iface);
		};
	};
	
	closedir(dir);
		                
	return status;
}

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			printf("ethernet: B_MODULE_INIT\n");
			return get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			
		case B_MODULE_UNINIT:
			printf("ethernet: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_interface_module_info nimi = {
	{
		NET_INTERFACE_MODULE_ROOT "ethernet/v0",
		0,
		std_ops
	},
	
	init,
	uninit,
	up,
	down,
	send,
	receive,
	control,
	get_hardware_address,
	get_multicast_addresses,
	set_multicast_addresses,
	set_mtu,
	set_promiscuous,
	set_media
};

_EXPORT module_info *modules[] = {
	(module_info*) &nimi,	// net_interface_module_info
	NULL
};
