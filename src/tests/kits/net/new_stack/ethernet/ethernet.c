/* generic_ethernet.c - generic ethernet devices layer module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/dirent.h>
#include <sys/stat.h>

#include <KernelExport.h>
#include <Drivers.h>

#include "net_stack.h"

enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,
	ETHER_INIT,
	ETHER_NONBLOCK,
	ETHER_ADDMULTI,
	ETHER_REMMULTI,
	ETHER_SETPROMISC,
	ETHER_GETFRAMESIZE
};

typedef struct generic_ethernet_device {
	int 				fd;
	int					max_frame_size;
	volatile thread_id	reader_thread;
} generic_ethernet_device;

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
status_t 	register_device(const char *path, void *cookie);
status_t 	device_reader(void *args);
status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;


// -------------------
status_t init(net_layer *me)
{
	printf("%s: initing layer\n", me->name);
	return B_OK;
}

status_t uninit(net_layer *me)
{
	generic_ethernet_device *ged = me->cookie;
	
	printf("%s: uniniting layer\n", me->name);
	
	close(ged->fd);
	free(me->cookie);

	return B_OK;
}


status_t enable(net_layer *me, bool enable)
{
	generic_ethernet_device *ged = me->cookie;

	if (enable) {
		// enable this layer
		thread_id tid;
		char name[B_OS_NAME_LENGTH * 2];

		if (ged->reader_thread != -1)
			// already up
			return B_OK;
		
		strncpy(name, me->name, sizeof(name));
		strncat(name, " reader", sizeof(name)); 
		tid = spawn_kernel_thread(device_reader, name, B_NORMAL_PRIORITY, me);
		if (tid < 0) {
			printf("%s: failed to start device reader thread -> %d [%s]\n",
				me->name, (int) tid, strerror(tid));
			return tid;
		};

		ged->reader_thread = tid;
		return resume_thread(tid);

	} else {

		status_t dummy;
	
		// disable this layer
		if (ged->reader_thread == -1)
			// already down
			return B_OK;
	
		send_signal_etc(ged->reader_thread, SIGTERM, 0);
		wait_for_thread(ged->reader_thread, &dummy);
		printf("%s: device reader stopped.\n", me->name);
		ged->reader_thread = -1;
		return B_OK;
	};
	
	return B_OK;
}


status_t output_buffer(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	// TODO!
	printf("%s: output_buffer:\n", me->name);
	g_stack->dump_buffer(buffer);

	g_stack->delete_buffer(buffer, false);
	return B_OK;
}


// #pragma mark -

status_t lookup_devices(char *root, void *cookie)
{
	DIR *dir;
	struct dirent *de;
	struct stat st;
	char path[1024];
	status_t status;

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
			printf("generic_ethernet: Can't stat(%s) entry! Skipping...\n", path);
			continue;
		};
		
		if (S_ISDIR(st.st_mode))
			status = lookup_devices(path, cookie);
		else if (S_ISCHR(st.st_mode)) {	// char device = driver!
			if (strcmp(de->d_name, "stack") == 0 ||		// OBOS stack driver
				strcmp(de->d_name, "server") == 0 ||	// OBOS server driver
				strcmp(de->d_name, "api") == 0)			// BONE stack driver
				continue;	// skip pseudo-entries

			status = register_device(path, cookie);
		};
	};
	
	closedir(dir);
	
	return status;
}


status_t register_device(const char *path, void *cookie)
{
	generic_ethernet_device *ged;
	int frame_size;
	ether_addr_t mac_address;
	int fd;
	status_t status;
	char *name;
	
	fd = open(path, O_RDWR);
	if (fd < B_OK) {
		printf("generic_ethernet: Unable to open %s device -> %d [%s]. Skipping...\n", path,
			fd, strerror(fd));
		return fd;
	};
				
	// Init the network card
	status = ioctl(fd, ETHER_INIT, NULL, 0);
	if (status < B_OK) {
		printf("generic_ethernet: Failed to init %s device -> %ld [%s]. Skipping...\n", path,
				status, strerror(status));
		close(fd);
		return status;
	};
			
	// get the MAC address...
	status = ioctl(fd, ETHER_GETADDR, &mac_address, 6);
	if (status < B_OK) {
		printf("generic_ethernet: Failed to get %s device MAC address -> %ld [%s]. Skipping...\n",
			path, status, strerror(status));
		close(fd);
		return status;
	};
			
	// Try to determine the MTU to use
	status = ioctl(fd, ETHER_GETFRAMESIZE, &frame_size, sizeof(frame_size));
	if (status < B_OK) {
		frame_size = ETHERMTU;
		printf("generic_ethernet: %s device don't support ETHER_GETFRAMESIZE; defaulting to %d\n",
		       path, frame_size);
	};

	printf("generic_ethernet: Found device '%s': MAC address: %02x:%02x:%02x:%02x:%02x:%02x, MTU: %d bytes\n",
			path,
			mac_address.byte[0], mac_address.byte[1],
			mac_address.byte[2], mac_address.byte[3],
			mac_address.byte[4], mac_address.byte[5], frame_size);

	ged = malloc(sizeof(*ged));
	if (!ged) {
		close(fd);
		return B_NO_MEMORY;
	};

	ged->fd = fd;
	ged->max_frame_size = frame_size;
	ged->reader_thread = -1;

	name = malloc(strlen("ethernet/") + strlen(path)); 
	strcpy(name, "ethernet/");
	strcat(name, path + strlen("/dev/net/"));

	return g_stack->register_layer(name, &nlmi, ged, NULL);
}

// ----------------------------------------------------
status_t device_reader(void *args)
{
	net_layer 				*me = args;
	generic_ethernet_device *ged = me->cookie;
	net_buffer 				*buffer;
	status_t status;
	void *frame;
	ssize_t sz;

	printf("%s: device reader started.\n", me->name);
	
	status = B_OK;
	while(1) {
		frame = malloc(ged->max_frame_size);
		if (!frame) {
			status = B_NO_MEMORY;
			break;
		};

		// read on frame at time
		sz = read(ged->fd, frame, ged->max_frame_size);
		if (sz >= B_OK) {
			buffer = g_stack->new_buffer();
			if (!buffer) {
				free(frame);
				status = B_NO_MEMORY;
				break;
			};
		
			g_stack->add_to_buffer(buffer, 0, frame, sz, (buffer_chunk_free_func) free);
			
			printf("%s: input buffer:\n", me->name);
			g_stack->dump_buffer(buffer);
/*			
			g_stack->add_buffer_attribut(buffer, "ethernet:header", FROM_BUFFER, 0, 14);
			g_stack->add_buffer_attribut(buffer, "ethernet:to", FROM_BUFFER, 0, 6);
			g_stack->add_buffer_attribut(buffer, "ethernet:from", FROM_BUFFER, 6, 6);
			g_stack->add_buffer_attribut(buffer, "ethernet:protocol", FROM_BUFFER | NETWORK_ORDER, 12, 2);
*/
			// strip ethernet header
			g_stack->remove_from_buffer(buffer, 0, 14);

			if (g_stack->push_buffer_up(me, buffer) != B_OK)
				// nobody above us *eat* this buffer, so we drop it ourself :-(
				g_stack->delete_buffer(buffer, false);
		};
	};
	
	ged->reader_thread = -1;
	return status;
}

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			
			printf("generic_ethernet: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
				
			lookup_devices("/dev/net", NULL);
			return B_OK;
		};
			
		case B_MODULE_UNINIT:
			printf("generic_ethernet: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_LAYER_MODULES_ROOT "interfaces/generic_ethernet",
		0,
		std_ops
	},
	
	init,
	uninit,
	enable,
	NULL,	// no control()
	output_buffer,
	NULL 	// interface layer: no input_buffer()
};

_EXPORT module_info *modules[] = {
	(module_info*) &nlmi,	// net_layer_module_info
	NULL
};
