/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <util/kernel_cpp.h>
#include <thread.h>
#include <file_cache.h>
#include <fs/fd.h>
#include <generic_syscall.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


//#define TRACE_CACHE_MODULE
#ifdef TRACE_CACHE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// generic syscall interface
#define CACHE_LOG_SYSCALLS "cache_log"
#define CACHE_LOG_SET_MODULE	1


struct cache_log {
	team_id		team;
	char		team_name[B_OS_NAME_LENGTH];
	mount_id	device;
	vnode_id	parent;
	vnode_id	node;
	union {
		char	file_name[B_FILE_NAME_LENGTH];
		struct {
			char	**args;
			int32	arg_count;
			team_id	parent;
		}		launch;
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
static struct cache_module_info *sCacheModule;


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

	Thread *thread = thread_get_current_thread();
	log->team = thread->team->id;
	strlcpy(log->team_name, thread->name, B_OS_NAME_LENGTH);

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

	if (sCacheModule != NULL && sCacheModule->node_opened != NULL)
		sCacheModule->node_opened(vnode, fdType, device, parent, node, name, size);
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

	if (sCacheModule != NULL && sCacheModule->node_closed != NULL)
		sCacheModule->node_closed(vnode, fdType, device, node, accessType);
}


static void
log_node_launched(size_t argCount, char * const *args)
{
	cache_log *log = get_log_entry();
	if (log == NULL)
		return;

	log->action = 'l';
	log->type = FDTYPE_FILE;

	log->launch.args = (char **)malloc(argCount * sizeof(char *));
	log->launch.arg_count = argCount;

	for (uint32 i = 0; i < argCount; i++) {
		if  (i == 0) {
			// cut off path from parent team name
			Team *team = thread_get_current_thread()->team;
			char name[B_OS_NAME_LENGTH];
			cpu_status state;

			state = disable_interrupts();
			GRAB_TEAM_LOCK();

			log->launch.parent = team->parent->id;
			strlcpy(name, team->parent->main_thread->name, B_OS_NAME_LENGTH);

			RELEASE_TEAM_LOCK();
			restore_interrupts(state);

			log->launch.args[0] = strdup(name);
		} else
			log->launch.args[i] = strdup(args[i]);

		// remove newlines from the arguments
		char *newline;
		while ((newline = strchr(log->launch.args[i], '\n')) != NULL) {
			*newline = ' ';
		}
	}

	if (vfs_get_cwd(&log->device, &log->parent) != B_OK) {
		log->device = -1;
		log->parent = -1;
	}

	TRACE(("log: added entry %s, <l> %ld:%Ld %s ...\n", log->team_name,
		log->device, log->parent, args[0]));

	put_log_entry(log);

	if (sCacheModule != NULL && sCacheModule->node_launched != NULL)
		sCacheModule->node_launched(argCount, args);
}


static status_t
log_control(const char *subsystem, uint32 function,
	void *buffer, size_t bufferSize)
{
	switch (function) {
		case CACHE_LOG_SET_MODULE:
		{
			cache_module_info *module = sCacheModule;

			// unset previous module

			if (sCacheModule != NULL) {
				sCacheModule = NULL;
				snooze(100000);	// 0.1 secs
				put_module(module->info.name);
			}

			// get new module, if any

			if (buffer == NULL)
				return B_OK;

			char name[B_FILE_NAME_LENGTH];
			if (!IS_USER_ADDRESS(buffer)
				|| user_strlcpy(name, (char *)buffer, B_FILE_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			if (strncmp(name, CACHE_MODULES_NAME, strlen(CACHE_MODULES_NAME)))
				return B_BAD_VALUE;

			TRACE(("log_control: set module %s!\n", name));

			status_t status = get_module(name, (module_info **)&module);
			if (status == B_OK)
				sCacheModule = module;

			return status;
		}
	}

	return B_BAD_HANDLER;
}


static void
log_writer_daemon(void *arg, int /*iteration*/)
{
	mutex_lock(&sLock);

	if (sCurrentEntry > kLogWriteThreshold || arg != NULL) {
		off_t fileSize = 0;
		struct stat stat;
		if (fstat(sLogFile, &stat) == 0) {
			// enlarge file, so that it can be written faster
			fileSize = stat.st_size;
			ftruncate(sLogFile, fileSize + 512 * 1024);
		} else
			stat.st_size = -1;

		for (uint32 i = 0; i < sCurrentEntry; i++) {
			cache_log *log = &sLogEntries[i];
			char line[1236];
			ssize_t length = 0;

			switch (log->action) {
				case 'l':	// launch
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" l %ld:%Ld %ld \"%s\" ",
								log->team, log->timestamp, log->team_name,
								log->device, log->parent, log->launch.parent,
								log->launch.args[0]);
					length = std::min(length, (ssize_t)sizeof(line) - 1);

					for (int32 j = 1; j < log->launch.arg_count; j++) {
						// write argument list one by one, so that we can deal
						// with very long argument lists
						ssize_t written = write(sLogFile, line, length);
						if (written != length) {
							dprintf("log: must drop log entries: %ld, %s!\n",
								written, strerror(written));
							break;
						} else
							fileSize += length;

						strlcpy(line, log->launch.args[j], sizeof(line));
						length = strlcat(line, "\xb0 ", sizeof(line));
					}

					if (length >= (ssize_t)sizeof(line))
						length = sizeof(line) - 1;

					line[length - 1] = '\n';
					break;

				case 'c':	// close
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" c%d %ld:%Ld %ld\n",
						log->team, log->timestamp, log->team_name, log->type,
						log->device, log->node, log->access_type);
					length = std::min(length, (ssize_t)sizeof(line) - 1);
					break;

				default:	// open, ?
					length = snprintf(line, sizeof(line), "%ld: %Ld \"%s\" %c%d %ld:%Ld:%Ld:\"%s\"\n",
						log->team, log->timestamp, log->team_name, log->action, log->type, log->device,
						log->parent, log->node, log->file_name);
					length = std::min(length, (ssize_t)sizeof(line) - 1);
					break;
			}

			ssize_t written = write(sLogFile, line, length);
			if (written != length) {
				dprintf("log: must drop log entries: %ld, %s!\n", written, strerror(written));
				break;
			} else
				fileSize += length;
		}

		// shrink file again to its real size
		if (stat.st_size != -1)
			ftruncate(sLogFile, fileSize);

		// need to free any launch log items (also if writing fails)

		for (uint32 i = 0; i < sCurrentEntry; i++) {
			cache_log *log = &sLogEntries[i];
			if (log->action != 'l')
				continue;

			for (int32 j = 0; j < log->launch.arg_count; j++)
				free(log->launch.args[j]);

			free(log->launch.args);
		}

		release_sem_etc(sLogEntrySem, sCurrentEntry, B_DO_NOT_RESCHEDULE);
		sCurrentEntry = 0;
	}

	mutex_unlock(&sLock);
}


static void
uninit_log(void)
{
	TRACE(("** log uninit - \n"));

	unregister_kernel_daemon(log_writer_daemon, NULL);
	unregister_generic_syscall(CACHE_LOG_SYSCALLS, 1);

	log_writer_daemon((void *)true, 0);
		// flush log entries

	delete_sem(sLogEntrySem);
	mutex_destroy(&sLock);
	close(sLogFile);

	if (sCacheModule != NULL)
		put_module(sCacheModule->info.name);

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
		mutex_init(&sLock, "log cache module");
		register_kernel_daemon(log_writer_daemon, NULL, kLogWriterFrequency);
		register_generic_syscall(CACHE_LOG_SYSCALLS, log_control, 1, 0);

		TRACE(("** - log init\n"));
		return B_OK;
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
