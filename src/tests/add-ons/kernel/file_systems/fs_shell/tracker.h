#ifndef TRACKER_H
#define TRACKER_H

#include "compat.h"

#include <SupportDefs.h>
#include <OS.h>
#include <AppDefs.h>
#include <sys/types.h>


#define FSH_KILL_TRACKER	'kill'
#define FSH_NOTIFY_LISTENER	'notl'

typedef struct update_message {
	int32	op;
	dev_t	device;
	dev_t	toDevice;
	ino_t	parentNode;
	ino_t	targetNode;
	ino_t	node;
	char	name[B_FILE_NAME_LENGTH];
} update_message;


extern port_id gTrackerPort;

extern
#ifdef __cplusplus
"C"
#endif
int32 tracker_loop(void *data);

#endif	/* TRACKER_H */
