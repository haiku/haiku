/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <util/kernel_cpp.h>
#include <thread.h>
#include <file_cache.h>
#include <fd.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


#define TRACE_CACHE_MODULE
#ifdef TRACE_CACHE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct cache_log {
	team_id		team;
	char		team_name[B_OS_NAME_LENGTH];
	mount_id	device;
	vnode_id	parent;
	vnode_id	node;
	union {
		char	file_name[B_FILE_NAME_LENGTH];
		struct {
			char	**array;
			int32	count;
		}		args;
		int32	access_type;
	};
	bigtime_t	timestamp;
	uint8		type;
	uint8		action;
};

static const bigtime_t kLogTimeout = 50000;	// 50 ms
static const int32 kLogWriterFrequency = 1;	// every tenth second
static const uint32 kNumLogEntries = 6000;
static const uint32 kLogWriteThreshold = 2000;

static struct cache_log sLogEntries[kNumLogEntries];
static uint32 sCurrentEntry;
static sem_id sLogEntrySem;
static struct mutex sLock;
static int sLogFile;


static void
put_log_entry(cache_log *log)
{
	mutex_unlock(&sLock);
}


static cache_log *
get_log_entry()
{
	if (acquire_sem_etc(sLogEntrySem, 1, B_RELATIVE_TIMEOUT, kLogTimeout) != B_OK) {
		dprintf("log: Dropped log entry!\n");
		return NULL;
	}

	mutex_lock(&sLock);
	cache_log *log = &sLogEntries[sCurrentEntry++];

	struct thread *thread = thread_get_current_thread();
	log->team = thread->team->id;
	strlcpy(log->team_name, thread->team->name, B_OS_NAME_LENGTH);

	log->timestamp = system_time();

	return log;
}


static void
log_node_opened(void *vnode, int32 fdType, mount_id device, vnode_id parent,
	vnode_id node, const char *name, off_t size)
{
	cache_log *log = get_log_entry();
	if (log == NULL)
		return;

	if (name != NULL)
		strlcpy(log->file_name, name, B_FILE_NAME_LENGTH);
	else {
		log->file_name[0] = '\0';
		vfs_get_vnode_name(vnode, log->file_name, B_FILE_NAME_LENGTH);
	}
	log->action = 'o';
	log->type = fdType;

	log->device = device;
	log->parent = parent;
	log->node = node;
	log->timestamp = system_time();

	TRACE(("log: added entry %s, <%c%d> %ld:%Ld:%Ld:%s\n", log->team_name, log->action,
		log->type, device, parent, node, log->file_name));

	put_log_entry(log);
}


static void
log_node_closed(void *vnode, int32 fdType, mount_id device, vnode_id node, int32 accessType)
{
	cache_log *log = get_log_entry();
	if (log == NULL)
		return;

	log->action = 'c';
	log->type = fdType;

	log->device = device;
	log->parent = -1;
	log->node = node;

	log->access_type = accessType;

	TRACE(("log: added entry %s, <c%d> %ld:%Ld, %ld\n",
		log->team_name, log->type, device, node, accessType));

	put_log_entry(log);
}


static void
log_node_launched(size_t argCount, char * const *args)
{
	cache_log *log = get_log_entry();
	if (log == NULL)
		return;

	log->action = 'l';
	log->type = FDTYPE_FILE;

	log->args.array = (char **)malloc(argCount * sizeof(char *));
	log->args.count = argCount;
	
	for (uint32 i = 0; i < argCount; i++) {
		log->args.array[i] = strdup(args[i]);

		// remove newlines from the arguments
		char *newline;
		while ((newline = strchr(log->args.array[i], '\n')) != NULL) {
			*newline = ' ';
		}
	}

	TRACE(("log: added entry %s, <l> %s ...\n", log->team_name, args[0]));

	put_log_entry(log);
}


static void
log_writer_daemon(void *arg, int /*iteration*/)
{
	mutex_lock(&sLock);

	if (sCurrentEntry > kLogWriteThreshold || arg != NULL) {
		for (uint32 i = 0; i < sCurrentEntry; i++) {
			cache_log *log = &sLogEntries[i];
			char line[1536];
			ssize_t length = 0;

			switch (log->action) {
				case 'l':	// launch
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" l ",
								log->team, log->timestamp, log->team_name);

					for (int32 j = 0; j < log->args.count; j++) {
						strlcat(line, log->args.array[j], sizeof(line));
						length = strlcat(line, " ", sizeof(line));
					}

					if (length >= (ssize_t)sizeof(line))
						length = sizeof(line) - 1;

					line[length - 1] = '\n';
					break;

				case 'c':	// close
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" c%d %ld:%Ld %ld\n",
						log->team, log->timestamp, log->team_name, log->type,
						log->device, log->node, log->access_type);
					break;

				default:	// open, ?
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" %c%d %ld:%Ld:%Ld:\"%s\"\n",
						log->team, log->timestamp, log->team_name, log->action, log->type, log->device,
						log->parent, log->node, log->file_name);
					break;
			}

			if (write(sLogFile, line, length) != length) {
				dprintf("log: must drop log entries!\n");
				break;
			}
		}

		// need to free any launch log items (also if writing fails)

		for (uint32 i = 0; i < sCurrentEntry; i++) {
			cache_log *log = &sLogEntries[i];
			if (log->action != 'l')
				continue;

			for (int32 j = 0; j < log->args.count; j++)
				free(log->args.array[j]);

			free(log->args.array);
		}

		release_sem_etc(sLogEntrySem, sCurrentEntry, B_DO_NOT_RESCHEDULE);
		sCurrentEntry = 0;
	}

	mutex_unlock(&sLock);
}


static void
uninit_log(void)
{
	unregister_kernel_daemon(log_writer_daemon, NULL);

	log_writer_daemon((void *)true, 0);
		// flush log entries

	delete_sem(sLogEntrySem);
	mutex_destroy(&sLock);
	close(sLogFile);

	TRACE(("** - log uninit\n"));
}


static status_t
init_log(void)
{
	TRACE(("** log init\n"));

	int32 number = 0;
	do {
		char name[B_FILE_NAME_LENGTH];
		snprintf(name, sizeof(name), "/var/log/cache_%03ld", number++);

		sLogFile = open(name, O_WRONLY | O_CREAT | O_EXCL, DEFFILEMODE);
	} while (sLogFile < 0 && errno == B_FILE_EXISTS);

	if (sLogFile < B_OK) {
		dprintf("log: opening log file failed: %s\n", strerror(errno));
		return sLogFile;
	}

	sLogEntrySem = create_sem(kNumLogEntries, "cache log entries");
	if (sLogEntrySem >= B_OK) {
		if (mutex_init(&sLock, "log cache module") >= B_OK) {
			register_kernel_daemon(log_writer_daemon, NULL, kLogWriterFrequency);
			TRACE(("** - log init\n"));
			return B_OK;
		}
		delete_sem(sLogEntrySem);
	}
	close(sLogFile);
	return B_ERROR;
}


static status_t
log_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_log();

		case B_MODULE_UNINIT:
			uninit_log();
			return B_OK;

		default:
			return B_ERROR;
	}
}


static struct cache_module_info sLogCacheModule = {
	{
		CACHE_MODULES_NAME "/log/v1",
		0,
		log_std_ops,
	},
	log_node_opened,
	log_node_closed,
	log_node_launched,
};


module_info *modules[] = {
	(module_info *)&sLogCacheModule,
	NULL
};
