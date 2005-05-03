#include "tracker.h"
#include "kprotos.h"

//#include <map>
#include <stdio.h>


port_id gTrackerPort;


/*static void
tracker_query_attributes(dev_t device, ino_t node, char *name)
{
}
*/

static void
tracker_query_file(dev_t device, ino_t parent, char *name)
{
	if (name == NULL || parent == 0)
		return;

	int fd = sys_open_entry_ref(1, device, parent, name, O_RDONLY, 0);
	if (fd < 0)	{
		printf("tracker: could not open file: %s\n", name);
		return;
	}

	struct my_stat stat;
	int status = sys_rstat(true, fd, NULL, &stat, 1);
	if (status < 0) {
		printf("tracker: could not stat file: %s\n", name);
	}

	sys_close(true, fd);
}

#if 0
static void
tracker_scan_files(void)
{
}
#endif

extern "C" int32
tracker_loop(void *data)
{
	// create global messaging port
	
	gTrackerPort = create_port(128, "fsh tracker port");
	if (gTrackerPort < FS_OK)
		return gTrackerPort;

	while (true) {
		update_message message;
		int32 code;
		status_t status = read_port(gTrackerPort, &code, &message, sizeof(message));
		if (status < FS_OK)
			continue;

		if (code == FSH_KILL_TRACKER)
			break;

		if (code == FSH_NOTIFY_LISTENER) {
			printf("tracker: notify listener received\n");
			if (message.op != B_ATTR_CHANGED && message.op != B_DEVICE_UNMOUNTED)
				tracker_query_file(message.device, message.parentNode, message.name);
		} else if (code == B_QUERY_UPDATE) {
			printf("tracker: query update received\n");
			tracker_query_file(message.device, message.parentNode, message.name);
		} else {
			printf("tracker: unknown code received: 0x%lx\n", code);
		}
	}

	delete_port(gTrackerPort);
	return FS_OK;
}
