/******************************************************************************
 *
 * Module Name: oshaiku - Haiku OSL interfaces
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2005, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/


/*
 * These interfaces are required in order to compile the ASL compiler under
 * BeOS/Haiku.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <OS.h>

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#include <vm.h>
#include <PCI.h>
extern pci_module_info *gPCIManager;
#include <dpc.h>
extern dpc_module_info *gDPC;
extern void *gDPCHandle;
#endif

#include "acpi.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"

#define _COMPONENT		ACPI_OS_SERVICES
ACPI_MODULE_NAME		("oshaiku")

extern FILE *			AcpiGbl_DebugFile;
FILE *					AcpiGbl_OutputFile;

static uint32 acpi_root = 0;

#define DEBUG_OSHAIKU	0 /* verbosity level 0 = off, 1 = normal, 2 = all */

#if DEBUG_OSHAIKU > 0
#	define DEBUG_FUNCTION() \
		dprintf("acpi[%ld]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#	define DEBUG_FUNCTION_F(x, y...) \
		dprintf("acpi[%ld]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);

#	if DEBUG_OSHAIKU > 1
#		define DEBUG_FUNCTION_V() \
			dprintf("acpi[%ld]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#		define DEBUG_FUNCTION_VF(x, y...) \
			dprintf("acpi[%ld]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);
#	else
#		define DEBUG_FUNCTION_V() \
		/* nothing */
#		define DEBUG_FUNCTION_VF(x, y...) \
		/* nothing */
#	endif
#else
#	define DEBUG_FUNCTION() \
		/* nothing */
#	define DEBUG_FUNCTION_F(x, y...) \
		/* nothing */
#	define DEBUG_FUNCTION_V() \
		/* nothing */
#	define DEBUG_FUNCTION_VF(x, y...) \
		/* nothing */
#endif

/******************************************************************************
 *
 * FUNCTION:	AcpiOsInitialize, AcpiOsTerminate
 *
 * PARAMETERS:	None
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Init and terminate. Nothing to do.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsInitialize(void)
{
#ifndef _KERNEL_MODE
	AcpiGbl_OutputFile = stdout;
#else
	AcpiGbl_OutputFile = NULL;
#endif
	DEBUG_FUNCTION();
	return AE_OK;
}


ACPI_STATUS
AcpiOsTerminate(void)
{
	DEBUG_FUNCTION();
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsGetRootPointer
 *
 * PARAMETERS:	Flags	- Logical or physical addressing mode
 *				Address	- Where the address is returned
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Gets the root pointer (RSDP)
 *
 *****************************************************************************/

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer(void)
{
#ifdef _KERNEL_MODE
	ACPI_SIZE address;
	ACPI_STATUS status;
	DEBUG_FUNCTION();
	if (acpi_root == 0) {
		status = AcpiFindRootPointer(&address);
		if (status == AE_OK)
			acpi_root = address;
	}
	dprintf("AcpiOsGetRootPointer returning %p\n", (void *)acpi_root);
	return acpi_root;
#else
	return (AeLocalGetRootPointer());
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsPredefinedOverride
 *
 * PARAMETERS:	InitVal		- Initial value of the predefined object
 *				NewVal		- The new value for the object
 *
 * RETURN:		Status, pointer to value. Null pointer returned if not
 *				overriding.
 *
 * DESCRIPTION:	Allow the OS to override predefined names
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal,
	ACPI_STRING *NewVal)
{
	DEBUG_FUNCTION();
	if (!InitVal || !NewVal)
		return AE_BAD_PARAMETER;

	*NewVal = NULL;
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsTableOverride
 *
 * PARAMETERS:	ExistingTable	- Header of current table (probably firmware)
 *				NewTable		- Where an entire new table is returned.
 *
 * RETURN:		Status, pointer to new table. Null pointer returned if no
 *				table is available to override
 *
 * DESCRIPTION:	Return a different version of a table if one is available
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
	ACPI_TABLE_HEADER **NewTable)
{
	DEBUG_FUNCTION();
	if (!ExistingTable || !NewTable)
		return AE_BAD_PARAMETER;

	*NewTable = NULL;

#ifdef _ACPI_EXEC_APP
	/* This code exercises the table override mechanism in the core */
	if (!ACPI_STRNCMP(ExistingTable->Signature, DSDT_SIG, ACPI_NAME_SIZE)) {
		/* override DSDT with itself */
		*NewTable = AcpiGbl_DbTablePtr;
	}

	return AE_OK;
#else
	return AE_NO_ACPI_TABLES;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsReadable
 *
 * PARAMETERS:	Pointer				- Area to be verified
 *				Length				- Size of area
 *
 * RETURN:		TRUE if readable for entire length
 *
 * DESCRIPTION:	Verify that a pointer is valid for reading
 *
 *****************************************************************************/

BOOLEAN
AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
	DEBUG_FUNCTION_F("addr: %p; length: %lu", Pointer, (size_t)Length);
	return TRUE;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsWritable
 *
 * PARAMETERS:	Pointer				- Area to be verified
 *				Length				- Size of area
 *
 * RETURN:		TRUE if writable for entire length
 *
 * DESCRIPTION:	Verify that a pointer is valid for writing
 *
 *****************************************************************************/

BOOLEAN
AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
	DEBUG_FUNCTION_F("addr: %p; length: %lu", Pointer, (size_t)Length);
	return TRUE;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsRedirectOutput
 *
 * PARAMETERS:	Destination			- file handle/pointer
 *
 * RETURN:		None
 *
 * DESCRIPTION:	Causes redirect of AcpiOsPrintf and AcpiOsVprintf
 *
 *****************************************************************************/

void
AcpiOsRedirectOutput(void *Destination)
{
	DEBUG_FUNCTION();
	AcpiGbl_OutputFile = Destination;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsPrintf
 *
 * PARAMETERS:	fmt, ...			Standard printf format
 *
 * RETURN:		None
 *
 * DESCRIPTION:	Formatted output
 *
 *****************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(const char *Fmt, ...)
{
	va_list Args;
	DEBUG_FUNCTION();
	va_start(Args, Fmt);
	AcpiOsVprintf(Fmt, Args);
	va_end(Args);
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsVprintf
 *
 * PARAMETERS:	fmt					Standard printf format
 *				args				Argument list
 *
 * RETURN:		None
 *
 * DESCRIPTION:	Formatted output with argument list pointer
 *
 *****************************************************************************/

void
AcpiOsVprintf(const char *Fmt, va_list Args)
{
#ifndef _KERNEL_MODE
	INT32 Count = 0;
	UINT8 Flags;

	Flags = AcpiGbl_DbOutputFlags;
	if (Flags & ACPI_DB_REDIRECTABLE_OUTPUT) {
		/* Output is directable to either a file (if open) or the console */
		if (AcpiGbl_DebugFile) {
			/* Output file is open, send the output there */
			Count = vfprintf (AcpiGbl_DebugFile, Fmt, Args);
		} else {
			/* No redirection, send output to console (once only!) */
			Flags |= ACPI_DB_CONSOLE_OUTPUT;
		}
	}

	if (Flags & ACPI_DB_CONSOLE_OUTPUT)
		Count = vfprintf (AcpiGbl_OutputFile, Fmt, Args);

#else
	static char outputBuffer[1024];
	vsnprintf(outputBuffer, 1024, Fmt, Args);
	dprintf("%s", outputBuffer);
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsGetLine
 *
 * PARAMETERS:	fmt					Standard printf format
 *				args				Argument list
 *
 * RETURN:		Actual bytes read
 *
 * DESCRIPTION:	Formatted input with argument list pointer
 *
 *****************************************************************************/

UINT32
AcpiOsGetLine(char *Buffer)
{
	UINT32 i = 0;

#ifndef _KERNEL_MODE
	UINT8 Temp;

	for (i = 0; ; i++) {
		scanf("%1c", &Temp);
		if (!Temp || Temp == '\n')
			break;

		Buffer[i] = Temp;
	}
#endif

	/* Null terminate the buffer */
	Buffer[i] = 0;

	/* Return the number of bytes in the string */
	DEBUG_FUNCTION_F("buffer: \"%s\"; result: %lu", Buffer, (uint32)i);
	return i;
}

/******************************************************************************
 *
 * FUNCTION:	AcpiOsMapMemory
 *
 * PARAMETERS:	where				Physical address of memory to be mapped
 *				length				How much memory to map
 *				there				Logical address of mapped memory
 *
 * RETURN:		Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION:	Map physical memory into caller's address space
 *
 *****************************************************************************/

void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS where, ACPI_SIZE length)
{
#ifdef _KERNEL_MODE
	void *there;
	area_id area = map_physical_memory("acpi_physical_mem_area", (void *)where,
		length, B_ANY_KERNEL_ADDRESS, 0, &there);
	DEBUG_FUNCTION_F("addr: 0x%08lx; length: %lu; mapped: %p; area: %ld",
		(addr_t)where, (size_t)length, there, area);
	if (area < 0) {
		dprintf("ACPI: cannot map memory at 0x%08x, length %d\n", where, length);
		return NULL;
	}

	return there;
#else
	return NULL;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsUnmapMemory
 *
 * PARAMETERS:	where				Logical address of memory to be unmapped
 *				length				How much memory to unmap
 *
 * RETURN:		None.
 *
 * DESCRIPTION:	Delete a previously created mapping. Where and Length must
 *				correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory(void *where, ACPI_SIZE length)
{
	DEBUG_FUNCTION_F("mapped: %p; length: %lu", where, (size_t)length);
	delete_area(area_for(where));
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsAllocate
 *
 * PARAMETERS:	Size				Amount to allocate, in bytes
 *
 * RETURN:		Pointer to the new allocation. Null on error.
 *
 * DESCRIPTION:	Allocate memory. Algorithm is dependent on the OS.
 *
 *****************************************************************************/

void *
AcpiOsAllocate(ACPI_SIZE size)
{
	void *result = malloc((size_t)size);
	DEBUG_FUNCTION_VF("result: %p", result);
	return result;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsFree
 *
 * PARAMETERS:	mem					Pointer to previously allocated memory
 *
 * RETURN:		None.
 *
 * DESCRIPTION:	Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/

void
AcpiOsFree(void *mem)
{
	DEBUG_FUNCTION_VF("mem: %p", mem);
	free(mem);
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsCreateSemaphore
 *
 * PARAMETERS:	InitialUnits		- Units to be assigned to the new semaphore
 *				OutHandle			- Where a handle will be returned
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Create an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits,
	ACPI_HANDLE *OutHandle)
{
	*OutHandle = (ACPI_HANDLE)create_sem(InitialUnits, "acpi_sem");
	DEBUG_FUNCTION_F("max: %lu; count: %lu; result: %ld",
		(uint32)MaxUnits, (uint32)InitialUnits, (sem_id)*OutHandle);
	if (*OutHandle < B_OK)
		return AE_ERROR;
	return AE_OK;
}

/******************************************************************************
 *
 * FUNCTION:	AcpiOsDeleteSemaphore
 *
 * PARAMETERS:	Handle				- Handle returned by AcpiOsCreateSemaphore
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Delete an OS semaphore
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsDeleteSemaphore(ACPI_HANDLE Handle)
{
	DEBUG_FUNCTION_F("sem: %ld", (sem_id)Handle);
	delete_sem((sem_id)Handle);
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsWaitSemaphore
 *
 * PARAMETERS:	Handle				- Handle returned by AcpiOsCreateSemaphore
 *				Units				- How many units to wait for
 *				Timeout				- How long to wait (in milliseconds)
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Wait for units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_HANDLE Handle, UINT32 Units, UINT16 Timeout)
{
	ACPI_STATUS result;
	DEBUG_FUNCTION_VF("sem: %ld; count: %lu; timeout: %u",
		(sem_id)Handle, (uint32)Units, (uint16)Timeout);
	if (Timeout != ACPI_WAIT_FOREVER) {
		switch (acquire_sem_etc((sem_id)Handle, Units, B_RELATIVE_TIMEOUT,
			(bigtime_t)Timeout * 1000)) {
			case B_TIMED_OUT:
				result = AE_TIME;
				break;
			case B_BAD_VALUE:
				result = AE_BAD_PARAMETER;
				break;
			case B_OK:
				result = AE_OK;
				break;
			default:
				result = AE_ERROR;
				break;
		}
	} else {
		result = acquire_sem_etc((sem_id)Handle, Units, 0, 0)
			== B_OK ? AE_OK : AE_BAD_PARAMETER;
	}

	DEBUG_FUNCTION_VF("result: %lu", (uint32)result);
	return result;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsSignalSemaphore
 *
 * PARAMETERS:	Handle				- Handle returned by AcpiOsCreateSemaphore
 *				Units				- Number of units to send
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Send units
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignalSemaphore(ACPI_HANDLE Handle, UINT32 Units)
{
	DEBUG_FUNCTION_VF("sem: %ld; count: %lu", (sem_id)Handle,
		(uint32)Units);
	release_sem_etc((sem_id)Handle, Units, 0);
	return AE_OK;
}


ACPI_STATUS
AcpiOsCreateLock(ACPI_HANDLE *OutHandle)
{
	*OutHandle = (ACPI_HANDLE)malloc(sizeof(spinlock));
	DEBUG_FUNCTION_F("result: %p", (spinlock *)*OutHandle);
	if (OutHandle == NULL)
		return AE_NO_MEMORY;

	*((spinlock *)(*OutHandle)) = 0;
	return AE_OK;
}

void
AcpiOsDeleteLock(ACPI_HANDLE Handle)
{
	DEBUG_FUNCTION();
	free(Handle);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock(ACPI_HANDLE Handle)
{
	cpu_status cpu;
	DEBUG_FUNCTION_F("spinlock: %p", (spinlock *)Handle);
	cpu = disable_interrupts();
	acquire_spinlock((spinlock *)Handle);
	return cpu;
}


void
AcpiOsReleaseLock(ACPI_HANDLE Handle, ACPI_CPU_FLAGS Flags)
{
	release_spinlock((spinlock *)Handle);
	restore_interrupts((cpu_status)Flags);
	DEBUG_FUNCTION_F("spinlock: %p", (spinlock *)Handle);
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:	InterruptNumber		Level handler should respond to.
 *				Isr					Address of the ACPI interrupt handler
 *				ExceptPtr			Where status is returned
 *
 * RETURN:		Handle to the newly installed handler.
 *
 * DESCRIPTION:	Install an interrupt handler. Used to install the ACPI
 *				OS-independent handler.
 *
 *****************************************************************************/

UINT32
AcpiOsInstallInterruptHandler(UINT32 InterruptNumber,
	ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
	DEBUG_FUNCTION_F("vector: %lu; handler: %p; data: %p",
		(uint32)InterruptNumber, ServiceRoutine, Context);
#ifdef _KERNEL_MODE
		/*	It so happens that the Haiku and ACPI-CA interrupt handler routines
			return the same values with the same meanings */
	return install_io_interrupt_handler(InterruptNumber,
		(interrupt_handler)ServiceRoutine, Context, 0) == B_OK ? AE_OK
		: AE_ERROR;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:	Handle				Returned when handler was installed
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Uninstalls an interrupt handler.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
	ACPI_OSD_HANDLER ServiceRoutine)
{
	DEBUG_FUNCTION_F("vector: %lu; handler: %p", (uint32)InterruptNumber,
		ServiceRoutine);
#ifdef _KERNEL_MODE
	panic("AcpiOsRemoveInterruptHandler()\n");
	/*remove_io_interrupt_handler(InterruptNumber,
		(interrupt_handler)ServiceRoutine, NULL);*/
		/* Crap. We don't get the Context argument back. */
	#warning Sketchy code!
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsExecute
 *
 * PARAMETERS:	Type			- Type of execution
 *				Function		- Address of the function to execute
 *				Context			- Passed as a parameter to the function
 *
 * RETURN:		Status.
 *
 * DESCRIPTION:	Execute a new thread
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function,
	void *Context)
{
	DEBUG_FUNCTION();
	switch (Type) {
		case OSL_GLOBAL_LOCK_HANDLER:
		case OSL_NOTIFY_HANDLER:
		case OSL_GPE_HANDLER:
		case OSL_DEBUGGER_THREAD:
		case OSL_EC_POLL_HANDLER:
		case OSL_EC_BURST_HANDLER:
			break;
	}

	if (gDPC->queue_dpc(gDPCHandle, Function, Context) != B_OK)
		return AE_ERROR;

	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsBreakpoint
 *
 * PARAMETERS:	Msg					Message to print
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Print a message and break to the debugger.
 *
 *****************************************************************************/

#if 0
ACPI_STATUS
AcpiOsBreakpoint(char *Msg)
{
	if (Msg != NULL)
		kernel_debugger ("AcpiOsBreakpoint: %s ****\n", Msg);
	else
		kernel_debugger ("At AcpiOsBreakpoint ****\n");

	return AE_OK;
}
#endif

/******************************************************************************
 *
 * FUNCTION:	AcpiOsStall
 *
 * PARAMETERS:	microseconds		To sleep
 *
 * RETURN:		Blocks until sleep is completed.
 *
 * DESCRIPTION:	Sleep at microsecond granularity
 *
 *****************************************************************************/

void
AcpiOsStall(UINT32 microseconds)
{
	DEBUG_FUNCTION_F("microseconds: %lu", (uint32)microseconds);
	spin(microseconds);
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsSleep
 *
 * PARAMETERS:	milliseconds		To sleep
 *
 * RETURN:		Blocks until sleep is completed.
 *
 * DESCRIPTION:	Sleep at millisecond granularity
 *
 *****************************************************************************/

void
AcpiOsSleep(ACPI_INTEGER milliseconds)
{
	DEBUG_FUNCTION_F("milliseconds: %lu", (uint32)milliseconds);
	snooze(milliseconds * 1000); /* Sleep for micro seconds */
}

/******************************************************************************
 *
 * FUNCTION:	AcpiOsGetTimer
 *
 * PARAMETERS:	None
 *
 * RETURN:		Current time in 100 nanosecond units
 *
 * DESCRIPTION:	Get the current system time
 *
 *****************************************************************************/

UINT64
AcpiOsGetTimer (void)
{
	DEBUG_FUNCTION();
	return system_time() * 10;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsValidateInterface
 *
 * PARAMETERS:	Interface			- Requested interface to be validated
 *
 * RETURN:		AE_OK if interface is supported, AE_SUPPORT otherwise
 *
 * DESCRIPTION:	Match an interface string to the interfaces supported by the
 *				host. Strings originate from an AML call to the _OSI method.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsValidateInterface(char *Interface)
{
	DEBUG_FUNCTION_F("interface: \"%s\"", Interface);
	return AE_SUPPORT;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsValidateAddress
 *
 * PARAMETERS:	SpaceId				- ACPI space ID
 *				Address				- Physical address
 *				Length				- Address length
 *
 * RETURN:		AE_OK if Address/Length is valid for the SpaceId. Otherwise,
 *				should return AE_AML_ILLEGAL_ADDRESS.
 *
 * DESCRIPTION:	Validate a system address via the host OS. Used to validate
 *				the addresses accessed by AML operation regions.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsValidateAddress(UINT8 SpaceId, ACPI_PHYSICAL_ADDRESS Address,
	ACPI_SIZE Length)
{
	DEBUG_FUNCTION_F("space: %u; addr: 0x%08lx; length: %lu",
		(uint8)SpaceId, (addr_t)Address, (size_t)Length);
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsReadPciConfiguration
 *
 * PARAMETERS:	PciId				Seg/Bus/Dev
 *				Register			Device Register
 *				Value				Buffer where value is placed
 *				Width				Number of bits
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Read data from PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, void *Value,
	UINT32 Width)
{
#ifdef _KERNEL_MODE
	UINT32 val = gPCIManager->read_pci_config(
		PciId->Bus, PciId->Device, PciId->Function, Register, Width / 8);
	DEBUG_FUNCTION();
	switch (Width) {
		case 8: *(UINT8 *)Value = val;
		case 16: *(UINT16 *)Value = val;
		case 32: *(UINT32 *)Value = val;
		default:
			dprintf("AcpiOsReadPciConfiguration unhandled value width: %u\n",
				Width);
			return AE_ERROR;
	}

	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsWritePciConfiguration
 *
 * PARAMETERS:	PciId				Seg/Bus/Dev
 *				Register			Device Register
 *				Value				Value to be written
 *				Width				Number of bits
 *
 * RETURN:		Status.
 *
 * DESCRIPTION:	Write data to PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
	ACPI_INTEGER Value, UINT32 Width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	gPCIManager->write_pci_config(
		PciId->Bus, PciId->Device, PciId->Function, Register, Width / 8, Value);
	return AE_OK;
#else
	return AE_ERROR;
#endif
}

/* TEMPORARY STUB FUNCTION */
void
AcpiOsDerivePciId(ACPI_HANDLE rhandle, ACPI_HANDLE chandle, ACPI_PCI_ID **PciId)
{
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsReadPort
 *
 * PARAMETERS:	Address				Address of I/O port/register to read
 *				Value				Where value is placed
 *				Width				Number of bits
 *
 * RETURN:		Value read from port
 *
 * DESCRIPTION:	Read data from an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("addr: 0x%08lx; width: %lu", (addr_t)Address,
		(uint32)Width);
	switch (Width) {
		case 8:
			*Value = gPCIManager->read_io_8(Address);
			break;

		case 16:
			*Value = gPCIManager->read_io_16(Address);
			break;

		case 32:
			*Value = gPCIManager->read_io_32(Address);
			break;

		default:
			dprintf("AcpiOsReadPort: unhandeld width: %u\n", Width);
			return AE_ERROR;
	}

	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsWritePort
 *
 * PARAMETERS:	Address				Address of I/O port/register to write
 *				Value				Value to write
 *				Width				Number of bits
 *
 * RETURN:		None
 *
 * DESCRIPTION:	Write data to an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("addr: 0x%08lx; value: %lu; width: %lu",
		(addr_t)Address, (uint32)Value, (uint32)Width);
	switch (Width) {
		case 8:
			gPCIManager->write_io_8(Address, Value);
			break;

		case 16:
			gPCIManager->write_io_16(Address,Value);
			break;

		case 32:
			gPCIManager->write_io_32(Address,Value);
			break;

		default:
			dprintf("AcpiOsWritePort: unhandeld width: %u\n", Width);
			return AE_ERROR;
	}

	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsReadMemory
 *
 * PARAMETERS:	Address				Physical Memory Address to read
 *				Value				Where value is placed
 *				Width				Number of bits
 *
 * RETURN:		Value read from physical memory address
 *
 * DESCRIPTION:	Read data from a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
#ifdef _KERNEL_MODE
	addr_t pageOffset = Address % B_PAGE_SIZE;
	addr_t virtualAddress;
	status_t error = vm_get_physical_page(Address - pageOffset,
		&virtualAddress, 0);
	DEBUG_FUNCTION();
	if (error != B_OK)
		return AE_ERROR;

	memcpy(Value, (void*)(virtualAddress + pageOffset), Width / 8);
	vm_put_physical_page(virtualAddress);
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsWriteMemory
 *
 * PARAMETERS:	Address				Physical Memory Address to write
 *				Value				Value to write
 *				Width				Number of bits
 *
 * RETURN:		None
 *
 * DESCRIPTION:	Write data to a physical memory address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT32 Value, UINT32 Width)
{
#ifdef _KERNEL_MODE
	addr_t pageOffset = Address % B_PAGE_SIZE;
	addr_t virtualAddress;
	status_t error = vm_get_physical_page(Address - pageOffset,
		&virtualAddress, 0);
	DEBUG_FUNCTION();
	if (error != B_OK)
		return AE_ERROR;

	memcpy((void*)(virtualAddress + pageOffset), &Value, Width / 8);
	vm_put_physical_page(virtualAddress);
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


UINT32
AcpiOsGetThreadId(void)
{
	return find_thread(NULL);
}


/******************************************************************************
 *
 * FUNCTION:	AcpiOsSignal
 *
 * PARAMETERS:	Function			ACPI CA signal function code
 *				Info				Pointer to function-dependent structure
 *
 * RETURN:		Status
 *
 * DESCRIPTION:	Miscellaneous functions
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignal(UINT32 Function, void *Info)
{
	DEBUG_FUNCTION();
	switch (Function) {
		case ACPI_SIGNAL_FATAL:
#ifdef _KERNEL_MODE
			if (Info != NULL)
				panic(Info);
			else
				panic("AcpiOsSignal: fatal");
			break;
#endif

		case ACPI_SIGNAL_BREAKPOINT:
			if (Info != NULL)
				AcpiOsPrintf ("AcpiOsBreakpoint: %s ****\n", Info);
			else
				AcpiOsPrintf ("At AcpiOsBreakpoint ****\n");
			break;
	}

	return AE_OK;
}
