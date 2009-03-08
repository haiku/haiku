/*
 * Copyright 2002-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <signal.h>
#include <stdio.h>
#include <string>

#include <KernelExport.h>
#include <module.h>

#include <low_resource_manager.h>
#include <fs/devfs.h>
#include <kscheduler.h>
#include <slab/Slab.h>


bool gDebugOutputEnabled = true;


thread_id
spawn_kernel_thread(thread_func func, const char *name, int32 priority,
	void *data)
{
	return spawn_thread(func, name, priority, data);
}


extern "C" status_t
devfs_unpublish_partition(const char *path)
{
	printf("unpublish partition: %s\n", path);
	return B_OK;
}


extern "C" status_t
devfs_publish_partition(const char *path, const partition_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	printf("publish partition: %s (device \"%s\", size %Ld)\n", path, info->device, info->size);
	return B_OK;
}


extern "C" int32
atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
#if __INTEL__
	int32 oldValue;
	asm volatile("lock; cmpxchg %%ecx, (%%edx)"
		: "=a" (oldValue) : "a" (testAgainst), "c" (newValue), "d" (value));
	return oldValue;
#else
#warn "atomic_test_and_set() won't work correctly!"
	int32 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;

	return oldValue;
#endif
}


extern "C" int
send_signal_etc(pid_t thread, uint signal, uint32 flags)
{
	return send_signal(thread, signal);
}


extern "C" int32
low_resource_state(uint32 resources)
{
	return B_NO_LOW_RESOURCE;
}


extern "C" void
low_resource(uint32 resource, uint64 requirements, uint32 flags,
	uint32 timeout)
{
}


extern "C" status_t
register_low_resource_handler(low_resource_func function, void *data,
	uint32 resources, int32 priority)
{
	return B_OK;
}


extern "C" status_t
unregister_low_resource_handler(low_resource_func function, void *data)
{
	return B_OK;
}


extern "C" void
panic(const char *format, ...)
{
	va_list args;

	puts("*** PANIC ***");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	putchar('\n');
	debugger("Kernel Panic");
}


extern "C" void
dprintf(const char *format,...)
{
	if (!gDebugOutputEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("\33[34m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
kprintf(const char *format,...)
{
	va_list args;
	va_start(args, format);
	printf("\33[35m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
dump_block(const char *buffer, int size, const char *prefix)
{
	const int DUMPED_BLOCK_SIZE = 16;
	int i;

	for (i = 0; i < size;) {
		int start = i;

		dprintf(prefix);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				dprintf(" ");

			if (i >= size)
				dprintf("  ");
			else
				dprintf("%02x", *(unsigned char *)(buffer + i));
		}
		dprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					dprintf(".");
				else
					dprintf("%c", c);
			} else
				break;
		}
		dprintf("\n");
	}
}


extern "C" status_t
user_memcpy(void *to, const void *from, size_t size)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	while (size--)
		*tmp++ = *s++;

	return 0;
}


extern "C" int
user_strcpy(char *to, const char *from)
{
	while ((*to++ = *from++) != '\0')
		;
	return 0;
}


/*!	\brief Copies at most (\a size - 1) characters from the string in \a from to
	the string in \a to, NULL-terminating the result.

	\param to Pointer to the destination C-string.
	\param from Pointer to the source C-string.
	\param size Size in bytes of the string buffer pointed to by \a to.

	\return strlen(\a from).
*/
extern "C" ssize_t
user_strlcpy(char *to, const char *from, size_t size)
{
	int from_length = 0;

	if (size > 0) {
		to[--size] = '\0';
		// copy
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	return from_length;
}


//	#pragma mark - Debugger


extern "C" uint64
parse_expression(const char* arg)
{
	return strtoull(arg, NULL, 0);
}


extern "C" bool
set_debug_variable(const char* variableName, uint64 value)
{
	return true;
}


extern "C" bool
print_debugger_command_usage(const char* command)
{
	return true;
}


extern "C" status_t
add_debugger_command_etc(const char* name, debugger_command_hook func,
	const char* description, const char* usage, uint32 flags)
{
	return B_OK;
}


extern "C" int
add_debugger_command(const char *name, int (*func)(int, char **),
	const char *desc)
{
	return B_OK;
}


extern "C" int
remove_debugger_command(const char * name, int (*func)(int, char **))
{
	return B_OK;
}


//	#pragma mark - Slab allocator


extern "C" void *
object_cache_alloc(object_cache *cache, uint32 flags)
{
	return malloc((size_t)cache);
}


extern "C" void
object_cache_free(object_cache *cache, void *object)
{
	free(object);
}


extern "C" object_cache *
create_object_cache_etc(const char *name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	return (object_cache*)objectSize;
}


extern "C" object_cache *
create_object_cache(const char *name, size_t objectSize,
	size_t alignment, void *cookie, object_cache_constructor constructor,
	object_cache_destructor)
{
	return (object_cache*)objectSize;
}


extern "C" void
delete_object_cache(object_cache *cache)
{
}


extern "C" void
object_cache_get_usage(object_cache *cache, size_t *_allocatedMemory)
{
	*_allocatedMemory = 100;
}


//	#pragma mark - Thread/scheduler functions


struct scheduler_ops kScheduler = {
	NULL,
	NULL,
	NULL,
	NULL,
};
struct scheduler_ops* gScheduler = &kScheduler;
