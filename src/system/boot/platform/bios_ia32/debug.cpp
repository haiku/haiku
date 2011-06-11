/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "debug.h"

#include <string.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <kernel.h>
#include <util/ring_buffer.h>

#include "keyboard.h"
#include "mmu.h"
#include "serial.h"


static const char* const kDebugSyslogSignature = "Haiku syslog";

static char sBuffer[16384];
static uint32 sBufferPosition;

static ring_buffer* sDebugSyslogBuffer = NULL;
static bool sPostCleanup = false;


static void
dprintf_args(const char *format, va_list args)
{
	char buffer[512];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);

	if (length >= (int)sizeof(buffer))
		length = sizeof(buffer) - 1;

	if (sPostCleanup && sDebugSyslogBuffer != NULL) {
		ring_buffer_write(sDebugSyslogBuffer, (const uint8*)buffer, length);
	} else if (sBufferPosition + length < sizeof(sBuffer)) {
		memcpy(sBuffer + sBufferPosition, buffer, length);
		sBufferPosition += length;
	}

	serial_puts(buffer, length);

	if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
		fprintf(stderr, "%s", buffer);
}


// #pragma mark -


/*!	This works only after console_init() was called.
*/
void
panic(const char *format, ...)
{
	va_list list;

	platform_switch_to_text_mode();

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	puts("\nPress key to reboot.");

	clear_key_buffer();
	wait_for_key();
	platform_exit();
}


void
dprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);
}


void
kprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);

	// print to console, if available
	if (stdout != NULL)
		vfprintf(stdout, format, args);

	// always print to serial line
	dprintf_args(format, args);

	va_end(args);
}


// #pragma mark -


void
debug_init_post_mmu(void)
{
	// allocate 1 MB memory at 63 MB
	addr_t base = 63 * 1024 * 1024;
	size_t size = 1024 * 1024;
	if (!mmu_allocate_physical(base, size))
		return;

	void* buffer = (void*)mmu_map_physical_memory(base, size,
		kDefaultPageFlags);
	if (buffer == NULL)
		return;

	// check whether there's a previous syslog we can recover
	size_t signatureLength = strlen(kDebugSyslogSignature);
	bool recover = memcmp(buffer, kDebugSyslogSignature, signatureLength) == 0;

	size -= signatureLength;
	buffer = (uint8*)buffer + ROUNDUP(signatureLength, sizeof(void*));

	sDebugSyslogBuffer = create_ring_buffer_etc(buffer, size,
		recover ? RING_BUFFER_INIT_FROM_BUFFER : 0);

	gKernelArgs.debug_output = sDebugSyslogBuffer;
	gKernelArgs.debug_size = sDebugSyslogBuffer->size;
}


void
debug_cleanup(void)
{
	if (sDebugSyslogBuffer != NULL) {
		size_t signatureLength = strlen(kDebugSyslogSignature);
		void* buffer
			= (void*)ROUNDDOWN((addr_t)sDebugSyslogBuffer, B_PAGE_SIZE);

		if (gKernelArgs.keep_debug_output_buffer) {
			// copy the output gathered so far into the ring buffer
			ring_buffer_clear(sDebugSyslogBuffer);
			ring_buffer_write(sDebugSyslogBuffer, (uint8*)sBuffer,
				sBufferPosition);

			memcpy(buffer, kDebugSyslogSignature, signatureLength);
		} else {
			// clear the signature
			memset(buffer, 0, signatureLength);
		}
	} else
		gKernelArgs.keep_debug_output_buffer = false;

	if (!gKernelArgs.keep_debug_output_buffer) {
		gKernelArgs.debug_output = kernel_args_malloc(sBufferPosition);
		if (gKernelArgs.debug_output != NULL) {
			memcpy(gKernelArgs.debug_output, sBuffer, sBufferPosition);
			gKernelArgs.debug_size = sBufferPosition;
		}
	}

	sPostCleanup = true;
}
