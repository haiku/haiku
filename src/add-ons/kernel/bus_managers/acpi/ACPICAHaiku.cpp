/******************************************************************************
 *
 * Module Name: oshaiku - Haiku OSL interfaces
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
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


#include <stdio.h>
#include <sys/cdefs.h>
#include <time.h>
#include <unistd.h>

#include <OS.h>

#ifdef _KERNEL_MODE
#	include <KernelExport.h>

#	include <dpc.h>
#	include <PCI.h>

#	include <boot_item.h>
#	include <kernel.h>
#	include <vm/vm.h>
#endif

__BEGIN_DECLS
#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"
__END_DECLS


ACPI_MODULE_NAME("Haiku ACPI Module")

#define _COMPONENT ACPI_OS_SERVICES

// verbosity level 0 = off, 1 = normal, 2 = all
#define DEBUG_OSHAIKU 0

#if DEBUG_OSHAIKU <= 0
// No debugging, do nothing
#	define DEBUG_FUNCTION()
#	define DEBUG_FUNCTION_F(x, y...)
#	define DEBUG_FUNCTION_V()
#	define DEBUG_FUNCTION_VF(x, y...)
#else
#	define DEBUG_FUNCTION() \
		dprintf("acpi[%ld]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#	define DEBUG_FUNCTION_F(x, y...) \
		dprintf("acpi[%ld]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);
#	if DEBUG_OSHAIKU == 1
// No verbose debugging, do nothing
#		define DEBUG_FUNCTION_V()
#		define DEBUG_FUNCTION_VF(x, y...)
#	else
// Full debugging
#		define DEBUG_FUNCTION_V() \
			dprintf("acpi[%ld]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#		define DEBUG_FUNCTION_VF(x, y...) \
			dprintf("acpi[%ld]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);
#	endif
#endif


#ifdef _KERNEL_MODE
extern pci_module_info *gPCIManager;
extern dpc_module_info *gDPC;
extern void *gDPCHandle;
#endif

extern FILE *AcpiGbl_DebugFile;
FILE *AcpiGbl_OutputFile;

static ACPI_PHYSICAL_ADDRESS sACPIRoot = 0;
static void *sInterruptHandlerData[32];


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInitialize, AcpiOsTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init and terminate.  Nothing to do.
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsInitialize()
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
AcpiOsTerminate()
{
	DEBUG_FUNCTION();
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      RSDP physical address
 *
 * DESCRIPTION: Gets the root pointer (RSDP)
 *
 *****************************************************************************/
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer()
{
#ifdef _KERNEL_MODE
	ACPI_PHYSICAL_ADDRESS address;
	ACPI_STATUS status = AE_OK;
	DEBUG_FUNCTION();
	if (sACPIRoot == 0) {
		sACPIRoot = (ACPI_PHYSICAL_ADDRESS)get_boot_item("ACPI_ROOT_POINTER", NULL);
		if (sACPIRoot == 0) {
			status = AcpiFindRootPointer(&address);
			if (status == AE_OK)
				sACPIRoot = address;
		}
	}
	return sACPIRoot;
#else
	return AeLocalGetRootPointer();
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPredefinedOverride
 *
 * PARAMETERS:  initVal     - Initial value of the predefined object
 *              newVal      - The new value for the object
 *
 * RETURN:      Status, pointer to value.  Null pointer returned if not
 *              overriding.
 *
 * DESCRIPTION: Allow the OS to override predefined names
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *initVal,
		ACPI_STRING *newVal)
{
	DEBUG_FUNCTION();
	if (!initVal || !newVal)
		return AE_BAD_PARAMETER;

	*newVal = NULL;
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsTableOverride
 *
 * PARAMETERS:  existingTable   - Header of current table (probably firmware)
 *              newTable        - Where an entire new table is returned.
 *
 * RETURN:      Status, pointer to new table.  Null pointer returned if no
 *              table is available to override
 *
 * DESCRIPTION: Return a different version of a table if one is available
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER *existingTable,
		ACPI_TABLE_HEADER **newTable)
{
	DEBUG_FUNCTION();
	if (!existingTable || !newTable)
		return AE_BAD_PARAMETER;

	*newTable = NULL;

#ifdef ACPI_EXEC_APP
	AeTableOverride(existingTable, newTable);
	return AE_OK;
#else
	return AE_NO_ACPI_TABLES;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPhysicalTableOverride
 *
 * PARAMETERS:  existingTable       - Header of current table (probably firmware)
 *              newAddress          - Where new table address is returned
 *                                    (Physical address)
 *              newTableLength      - Where new table length is returned
 *
 * RETURN:      Status, address/length of new table. Null pointer returned
 *              if no table is available to override.
 *
 * DESCRIPTION: Returns AE_SUPPORT, function not used in user space.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *existingTable,
	ACPI_PHYSICAL_ADDRESS *newAddress, UINT32 *newTableLength)
{
	DEBUG_FUNCTION();
    return (AE_SUPPORT);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRedirectOutput
 *
 * PARAMETERS:  destination         - An open file handle/pointer
 *
 * RETURN:      None
 *
 * DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
 *
 *****************************************************************************/
void
AcpiOsRedirectOutput(void *destination)
{
	DEBUG_FUNCTION();
	AcpiGbl_OutputFile = (FILE*)destination;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPrintf
 *
 * PARAMETERS:  fmt, ...            Standard printf format
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(const char *fmt, ...)
{
	va_list args;

	DEBUG_FUNCTION();
	va_start(args, fmt);
	AcpiOsVprintf(fmt, args);
	va_end(args);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsVprintf
 *
 * PARAMETERS:  fmt                 Standard printf format
 *              args                Argument list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output with argument list pointer
 *
 *****************************************************************************/
void
AcpiOsVprintf(const char *fmt, va_list args)
{
#ifndef _KERNEL_MODE
	UINT8 flags;

	flags = AcpiGbl_DbOutputFlags;
	if (flags & ACPI_DB_REDIRECTABLE_OUTPUT) {
		// Output is directable to either a file (if open) or the console
		if (AcpiGbl_DebugFile) {
			// Output file is open, send the output there
			vfprintf(AcpiGbl_DebugFile, fmt, args);
		} else {
			// No redirection, send output to console (once only!)
			flags |= ACPI_DB_CONSOLE_OUTPUT;
		}
	}

	if (flags & ACPI_DB_CONSOLE_OUTPUT) {
		vfprintf(AcpiGbl_OutputFile, fmt, args);
    }
#else
	static char outputBuffer[1024];
	vsnprintf(outputBuffer, 1024, fmt, args);
	dprintf("%s", outputBuffer);
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetLine
 *
 * PARAMETERS:  fmt                 Standard printf format
 *              args                Argument list
 *
 * RETURN:      Actual bytes read
 *
 * DESCRIPTION: Formatted input with argument list pointer
 *
 *****************************************************************************/
UINT32
AcpiOsGetLine(char *buffer)
{
	uint32 i = 0;

#ifndef _KERNEL_MODE
	uint8 temp;

	for (i = 0; ; i++) {
		scanf("%1c", &temp);
		if (!temp || temp == '\n')
			break;

		buffer[i] = temp;
	}
#endif

	buffer[i] = 0;
	DEBUG_FUNCTION_F("buffer: \"%s\"; result: %lu", buffer, i);
	return i;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  where               Physical address of memory to be mapped
 *              length              How much memory to map
 *
 * RETURN:      Pointer to mapped memory.  Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/
void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS where, ACPI_SIZE length)
{
#ifdef _KERNEL_MODE
	void *there;
	area_id area = map_physical_memory("acpi_physical_mem_area",
		(phys_addr_t)where, length, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &there);

	DEBUG_FUNCTION_F("addr: 0x%08lx; length: %lu; mapped: %p; area: %ld",
		(addr_t)where, (size_t)length, there, area);
	if (area < 0) {
		dprintf("ACPI: cannot map memory at 0x%" B_PRIu64 ", length %"
			B_PRIu64 "\n", (uint64)where, (uint64)length);
		return NULL;
	}
	return there;
#else
	return NULL;
#endif

	// return ACPI_TO_POINTER((ACPI_SIZE) where);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  where               Logical address of memory to be unmapped
 *              length              How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping.  Where and Length must
 *              correspond to a previous mapping exactly.
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
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  size                Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation.  Null on error.
 *
 * DESCRIPTION: Allocate memory.  Algorithm is dependent on the OS.
 *
 *****************************************************************************/
void *
AcpiOsAllocate(ACPI_SIZE size)
{
	void *mem = (void *) malloc(size);
	DEBUG_FUNCTION_VF("result: %p", mem);
	return mem;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  mem                 Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
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
 * FUNCTION:    AcpiOsCreateSemaphore
 *
 * PARAMETERS:  initialUnits        - Units to be assigned to the new semaphore
 *              outHandle           - Where a handle will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an OS semaphore
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 maxUnits, UINT32 initialUnits,
		ACPI_SEMAPHORE *outHandle)
{
	if (!outHandle)
    	return AE_BAD_PARAMETER;

	*outHandle = create_sem(initialUnits, "acpi_sem");
	DEBUG_FUNCTION_F("max: %lu; count: %lu; result: %ld",
		maxUnits, initialUnits, *outHandle);

	if (*outHandle >= B_OK)
		return AE_OK;

	return *outHandle == B_BAD_VALUE ? AE_BAD_PARAMETER : AE_NO_MEMORY;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsDeleteSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete an OS semaphore
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle)
{
	DEBUG_FUNCTION_F("sem: %ld", handle);
	return delete_sem(handle) == B_OK ? AE_OK : AE_BAD_PARAMETER;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *              units               - How many units to wait for
 *              timeout             - How long to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Wait for units
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 units, UINT16 timeout)
{
	ACPI_STATUS result = AE_OK;
	DEBUG_FUNCTION_VF("sem: %ld; count: %lu; timeout: %u",
		handle, units, timeout);

	if (timeout == ACPI_WAIT_FOREVER) {
		result = acquire_sem_etc(handle, units, 0, 0)
			== B_OK ? AE_OK : AE_BAD_PARAMETER;
	} else {
		switch (acquire_sem_etc(handle, units, B_RELATIVE_TIMEOUT,
			(bigtime_t)timeout * 1000)) {
			case B_OK:
				result = AE_OK;
				break;
			case B_INTERRUPTED:
			case B_TIMED_OUT:
			case B_WOULD_BLOCK:
				result = AE_TIME;
				break;
			case B_BAD_VALUE:
			default:
				result = AE_BAD_PARAMETER;
				break;
		}
	}
	DEBUG_FUNCTION_VF("sem: %ld; count: %lu; timeout: %u result: %lu",
		handle, units, timeout, (uint32)result);
	return result;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignalSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *              units               - Number of units to send
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Send units
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 units)
{
	status_t result;
	DEBUG_FUNCTION_VF("sem: %ld; count: %lu", handle, units);
	// We can be called from interrupt handler, so don't reschedule
	result = release_sem_etc(handle, units, B_DO_NOT_RESCHEDULE);
	return result == B_OK ? AE_OK : AE_BAD_PARAMETER;
}


/******************************************************************************
 *
 * FUNCTION:    Spinlock interfaces
 *
 * DESCRIPTION: Map these interfaces to semaphore interfaces
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsCreateLock(ACPI_SPINLOCK *outHandle)
{
	*outHandle = (ACPI_SPINLOCK) malloc(sizeof(spinlock));
	DEBUG_FUNCTION_F("result: %p", *outHandle);
	if (*outHandle == NULL)
		return AE_NO_MEMORY;

	B_INITIALIZE_SPINLOCK(*outHandle);
	return AE_OK;
}


void
AcpiOsDeleteLock(ACPI_SPINLOCK handle)
{
	DEBUG_FUNCTION();
	free((void*)handle);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock(ACPI_SPINLOCK handle)
{
	cpu_status cpu;
	DEBUG_FUNCTION_F("spinlock: %p", handle);
	cpu = disable_interrupts();
	acquire_spinlock(handle);
	return cpu;
}


void
AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags)
{
	release_spinlock(handle);
	restore_interrupts(flags);
	DEBUG_FUNCTION_F("spinlock: %p", handle);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:  interruptNumber     Level handler should respond to.
 *              Isr                 Address of the ACPI interrupt handler
 *              ExceptPtr           Where status is returned
 *
 * RETURN:      Handle to the newly installed handler.
 *
 * DESCRIPTION: Install an interrupt handler.  Used to install the ACPI
 *              OS-independent handler.
 *
 *****************************************************************************/
UINT32
AcpiOsInstallInterruptHandler(UINT32 interruptNumber,
		ACPI_OSD_HANDLER serviceRoutine, void *context)
{
	status_t result;
	DEBUG_FUNCTION_F("vector: %lu; handler: %p context %p",
		interruptNumber, serviceRoutine, context);

#ifdef _KERNEL_MODE
	// It so happens that the Haiku and ACPI-CA interrupt handler routines
	// return the same values with the same meanings
	sInterruptHandlerData[interruptNumber] = context;
	result = install_io_interrupt_handler(interruptNumber,
		(interrupt_handler)serviceRoutine, context, 0);

	DEBUG_FUNCTION_F("vector: %lu; handler: %p context %p returned %d",
		interruptNumber, serviceRoutine, context, result);

	return result == B_OK ? AE_OK : AE_BAD_PARAMETER;
#else
	return AE_BAD_PARAMETER;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:  Handle              Returned when handler was installed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Uninstalls an interrupt handler.
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 interruptNumber,
		ACPI_OSD_HANDLER serviceRoutine)
{
	DEBUG_FUNCTION_F("vector: %lu; handler: %p", interruptNumber,
		serviceRoutine);
#ifdef _KERNEL_MODE
	remove_io_interrupt_handler(interruptNumber,
		(interrupt_handler) serviceRoutine,
		sInterruptHandlerData[interruptNumber]);
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsExecute
 *
 * PARAMETERS:  type            - Type of execution
 *              function        - Address of the function to execute
 *              context         - Passed as a parameter to the function
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Execute a new thread
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK  function,
		void *context)
{
	DEBUG_FUNCTION();
/* TODO: Prioritize urgent?
	switch (type) {
		case OSL_GLOBAL_LOCK_HANDLER:
		case OSL_NOTIFY_HANDLER:
		case OSL_GPE_HANDLER:
		case OSL_DEBUGGER_THREAD:
		case OSL_EC_POLL_HANDLER:
		case OSL_EC_BURST_HANDLER:
			break;
	}
*/

	if (gDPC->queue_dpc(gDPCHandle, function, context) != B_OK) {
		DEBUG_FUNCTION_F("Serious failure in AcpiOsExecute! function: %p",
			function);
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsStall
 *
 * PARAMETERS:  microseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at microsecond granularity
 *
 *****************************************************************************/
void
AcpiOsStall(UINT32 microseconds)
{
	DEBUG_FUNCTION_F("microseconds: %lu", microseconds);
	if (microseconds)
		spin(microseconds);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSleep
 *
 * PARAMETERS:  milliseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at millisecond granularity
 *
 *****************************************************************************/
void
AcpiOsSleep(ACPI_INTEGER milliseconds)
{
	DEBUG_FUNCTION_F("milliseconds: %lu", milliseconds);
	if (gKernelStartup)
		spin(milliseconds * 1000);
	else
		snooze(milliseconds * 1000);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTimer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Current time in 100 nanosecond units
 *
 * DESCRIPTION: Get the current system time
 *
 *****************************************************************************/
UINT64
AcpiOsGetTimer()
{
	DEBUG_FUNCTION();
	return system_time() * 10;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPciConfiguration
 *
 * PARAMETERS:  pciId               Seg/Bus/Dev
 *              reg                 Device Register
 *              value               Buffer where value is placed
 *              width               Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from PCI configuration space
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID *pciId, UINT32 reg, UINT64 *value,
		UINT32 width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();

	switch (width) {
		case 8:
		case 16:
		case 32:
			*value = gPCIManager->read_pci_config(
				pciId->Bus, pciId->Device, pciId->Function, reg, width / 8);
			break;
		default:
			return AE_ERROR;
	}
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePciConfiguration
 *
 * PARAMETERS:  pciId               Seg/Bus/Dev
 *              reg                 Device Register
 *              value               Value to be written
 *              width               Number of bits
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Write data to PCI configuration space
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID *pciId, UINT32 reg,
		ACPI_INTEGER value, UINT32 width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	gPCIManager->write_pci_config(
		pciId->Bus, pciId->Device, pciId->Function, reg, width / 8, value);
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPort
 *
 * PARAMETERS:  address             Address of I/O port/register to read
 *              Value               Where value is placed
 *              width               Number of bits
 *
 * RETURN:      Value read from port
 *
 * DESCRIPTION: Read data from an I/O port or register
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS address, UINT32 *value, UINT32 width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("addr: 0x%08lx; width: %lu", (addr_t)address, width);
	switch (width) {
		case 8:
			*value = gPCIManager->read_io_8(address);
			break;

		case 16:
			*value = gPCIManager->read_io_16(address);
			break;

		case 32:
			*value = gPCIManager->read_io_32(address);
			break;

		default:
			return AE_ERROR;
	}

	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePort
 *
 * PARAMETERS:  address             Address of I/O port/register to write
 *              value               Value to write
 *              width               Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to an I/O port or register
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS address, UINT32 value, UINT32 width)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("addr: 0x%08lx; value: %lu; width: %lu",
		(addr_t)address, value, width);
	switch (width) {
		case 8:
			gPCIManager->write_io_8(address, value);
			break;

		case 16:
			gPCIManager->write_io_16(address,value);
			break;

		case 32:
			gPCIManager->write_io_32(address,value);
			break;

		default:
			return AE_ERROR;
	}

	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadMemory
 *
 * PARAMETERS:  address             Physical Memory Address to read
 *              value               Where value is placed
 *              width               Number of bits
 *
 * RETURN:      Value read from physical memory address
 *
 * DESCRIPTION: Read data from a physical memory address
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 *value, UINT32 width)
{
#ifdef _KERNEL_MODE
	if (vm_memcpy_from_physical(value, (phys_addr_t)address, width / 8, false)
		!= B_OK) {
		return AE_ERROR;
	}
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWriteMemory
 *
 * PARAMETERS:  address             Physical Memory Address to write
 *              value               Value to write
 *              width               Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to a physical memory address
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 value, UINT32 width)
{
#ifdef _KERNEL_MODE
	if (vm_memcpy_to_physical((phys_addr_t)address, &value, width / 8, false)
			!= B_OK) {
		return AE_ERROR;
	}
	return AE_OK;
#else
	return AE_ERROR;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadable
 *
 * PARAMETERS:  pointer             - Area to be verified
 *              length              - Size of area
 *
 * RETURN:      TRUE if readable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for reading
 *
 *****************************************************************************/
BOOLEAN
AcpiOsReadable(void *pointer, ACPI_SIZE length)
{
#ifdef _KERNEL_MODE
	return true;
#else
	area_id id;
	area_info info;

	DEBUG_FUNCTION_F("addr: %p; length: %lu", pointer, (size_t)length);

	id = area_for(pointer);
	if (id == B_ERROR) return false;
	if (get_area_info(id, &info) != B_OK) return false;
	return (info.protection & B_READ_AREA) != 0 &&
			pointer + length <= info.address + info.ram_size;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritable
 *
 * PARAMETERS:  pointer             - Area to be verified
 *              length              - Size of area
 *
 * RETURN:      TRUE if writable for entire length
 *
 * DESCRIPTION: Verify that a pointer is valid for writing
 *
 *****************************************************************************/
BOOLEAN
AcpiOsWritable(void *pointer, ACPI_SIZE length)
{
#ifdef _KERNEL_MODE
	return true;
#else
	area_id id;
	area_info info;

	DEBUG_FUNCTION_F("addr: %p; length: %lu", pointer, (size_t)length);

	id = area_for(pointer);
	if (id == B_ERROR) return false;
	if (get_area_info(id, &info) != B_OK) return false;
	return (info.protection & B_READ_AREA) != 0 &&
			(info.protection & B_WRITE_AREA) != 0 &&
			pointer + length <= info.address + info.ram_size;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetThreadId
 *
 * PARAMETERS:  None
 *
 * RETURN:      Id of the running thread
 *
 * DESCRIPTION: Get the Id of the current (running) thread
 *
 * NOTE:        The environment header should contain this line:
 *                  #define ACPI_THREAD_ID pthread_t
 *
 *****************************************************************************/
ACPI_THREAD_ID
AcpiOsGetThreadId()
{
	thread_id thread = find_thread(NULL);
	// TODO: We arn't allowed threads with id 0, handle this case.
	// ACPI treats a 0 return as an error,
	// but we are thread 0 in early boot
	return thread;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignal
 *
 * PARAMETERS:  function            ACPI CA signal function code
 *              info                Pointer to function-dependent structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Miscellaneous functions. Example implementation only.
 *
 *****************************************************************************/
ACPI_STATUS
AcpiOsSignal(UINT32 function, void *info)
{
	DEBUG_FUNCTION();

	switch (function) {
		case ACPI_SIGNAL_FATAL:
#ifdef _KERNEL_MODE
			panic(info == NULL ? "AcpiOsSignal: fatal" : (const char*)info);
			break;
#endif
		case ACPI_SIGNAL_BREAKPOINT:
			if (info != NULL)
				AcpiOsPrintf("AcpiOsBreakpoint: %s ****\n", info);
			else
				AcpiOsPrintf("At AcpiOsBreakpoint ****\n");
			break;
	}

	return AE_OK;
}


/*
 * Adapted from FreeBSD since the documentation of its intended impl
 * is lacking.
 *  Section 5.2.10.1: global lock acquire/release functions */
#define GL_ACQUIRED     (-1)
#define GL_BUSY         0
#define GL_BIT_PENDING  0x01
#define GL_BIT_OWNED    0x02
#define GL_BIT_MASK     (GL_BIT_PENDING | GL_BIT_OWNED)


/*
 * Adapted from FreeBSD since the documentation of its intended impl
 * is lacking.
 * Acquire the global lock.  If busy, set the pending bit.  The caller
 * will wait for notification from the BIOS that the lock is available
 * and then attempt to acquire it again.
 */
int
AcpiOsAcquireGlobalLock(uint32 *lock)
{
	uint32 newValue;
	uint32 oldValue;

	do {
		oldValue = *lock;
		newValue = ((oldValue & ~GL_BIT_MASK) | GL_BIT_OWNED) |
				((oldValue >> 1) & GL_BIT_PENDING);
		atomic_test_and_set((int32*)lock, newValue, oldValue);
	} while (*lock == oldValue);
	return ((newValue < GL_BIT_MASK) ? GL_ACQUIRED : GL_BUSY);
}


/*
 * Adapted from FreeBSD since the documentation of its intended impl
 * is lacking.
 * Release the global lock, returning whether there is a waiter pending.
 * If the BIOS set the pending bit, OSPM must notify the BIOS when it
 * releases the lock.
 */
int
AcpiOsReleaseGlobalLock(uint32 *lock)
{
	uint32 newValue;
	uint32 oldValue;

	do {
		oldValue = *lock;
		newValue = oldValue & ~GL_BIT_MASK;
		atomic_test_and_set((int32*)lock, newValue, oldValue);
	} while (*lock == oldValue);
	return (oldValue & GL_BIT_PENDING);
}


ACPI_STATUS
AcpiOsCreateMutex(ACPI_MUTEX* outHandle)
{
	*outHandle = (ACPI_MUTEX) malloc(sizeof(mutex));
	DEBUG_FUNCTION_F("result: %p", *outHandle);
	if (*outHandle == NULL)
		return AE_NO_MEMORY;

	mutex_init(*outHandle, "acpi mutex");
	return AE_OK;
}


void
AcpiOsDeleteMutex(ACPI_MUTEX handle)
{
	DEBUG_FUNCTION_F("mutex: %ld", handle);
	mutex_destroy(handle);
	free((void*)handle);
}


ACPI_STATUS
AcpiOsAcquireMutex(ACPI_MUTEX handle, UINT16 timeout)
{
	ACPI_STATUS result = AE_OK;
	DEBUG_FUNCTION_VF("mutex: %ld; timeout: %u", handle, timeout);

	if (timeout == ACPI_WAIT_FOREVER)
		result = mutex_lock(handle) == B_OK ? AE_OK : AE_BAD_PARAMETER;
	else {
		switch (mutex_lock_with_timeout(handle, B_RELATIVE_TIMEOUT,
			(bigtime_t)timeout * 1000)) {
			case B_OK:
				result = AE_OK;
				break;
			case B_INTERRUPTED:
			case B_TIMED_OUT:
			case B_WOULD_BLOCK:
				result = AE_TIME;
				break;
			case B_BAD_VALUE:
			default:
				result = AE_BAD_PARAMETER;
				break;
		}
	}
	DEBUG_FUNCTION_VF("mutex: %ld; timeout: %u result: %lu",
		handle, timeout, (uint32)result);
	return result;
}


void
AcpiOsReleaseMutex(ACPI_MUTEX handle)
{
	DEBUG_FUNCTION_F("mutex: %p", handle);
	mutex_unlock(handle);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitEventsComplete
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Wait for all asynchronous events to complete. This
 *              implementation does nothing.
 *
 *****************************************************************************/
void
AcpiOsWaitEventsComplete()
{
	//TODO: FreeBSD See description.
	return;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsEnterSleep
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter
 *              RegaValue           - Register A value
 *              RegbValue           - Register B value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: A hook before writing sleep registers to enter the sleep
 *              state. Return AE_CTRL_TERMINATE to skip further sleep register
 *              writes.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsEnterSleep (
	UINT8                   SleepState,
	UINT32                  RegaValue,
	UINT32                  RegbValue)
{
	return (AE_OK);
}
