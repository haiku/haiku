/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BreakpointManager.h"

#include <algorithm>

#include <AutoDeleter.h>

#include <commpage_defs.h>
#include <kernel.h>
#include <util/AutoLock.h>
#include <vm/vm.h>


//#define TRACE_BREAKPOINT_MANAGER
#ifdef TRACE_BREAKPOINT_MANAGER
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif


// soft limit for the number of breakpoints
const int32 kMaxBreakpointCount = 10240;


BreakpointManager::InstalledBreakpoint::InstalledBreakpoint(addr_t address)
	:
	breakpoint(NULL),
	address(address)
{
}


// #pragma mark -


BreakpointManager::BreakpointManager()
	:
	fBreakpointCount(0),
	fWatchpointCount(0)
{
	rw_lock_init(&fLock, "breakpoint manager");
}


BreakpointManager::~BreakpointManager()
{
	WriteLocker locker(fLock);

	// delete the installed breakpoint objects
	BreakpointTree::Iterator it = fBreakpoints.GetIterator();
	while (InstalledBreakpoint* installedBreakpoint = it.Next()) {
		it.Remove();

		// delete underlying software breakpoint
		if (installedBreakpoint->breakpoint->software)
			delete installedBreakpoint->breakpoint;

		delete installedBreakpoint;
	}

	// delete the watchpoints
	while (InstalledWatchpoint* watchpoint = fWatchpoints.RemoveHead())
		delete watchpoint;

	// delete the hardware breakpoint objects
	while (Breakpoint* breakpoint = fHardwareBreakpoints.RemoveHead())
		delete breakpoint;

	rw_lock_destroy(&fLock);
}


status_t
BreakpointManager::Init()
{
	// create objects for the hardware breakpoints
	for (int32 i = 0; i < DEBUG_MAX_BREAKPOINTS; i++) {
		Breakpoint* breakpoint = new(std::nothrow) Breakpoint;
		if (breakpoint == NULL)
			return B_NO_MEMORY;

		breakpoint->address = 0;
		breakpoint->installedBreakpoint = NULL;
		breakpoint->used = false;
		breakpoint->software = false;

		fHardwareBreakpoints.Add(breakpoint);
	}

	return B_OK;
}


status_t
BreakpointManager::InstallBreakpoint(void* _address)
{
	const addr_t address = (addr_t)_address;

	WriteLocker locker(fLock);

	if (fBreakpointCount >= kMaxBreakpointCount)
		return B_BUSY;

	// check whether there's already a breakpoint at the address
	InstalledBreakpoint* installed = fBreakpoints.Lookup(address);
	if (installed != NULL)
		return B_BAD_VALUE;

	// create the breakpoint object
	installed = new(std::nothrow) InstalledBreakpoint(address);
	if (installed == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<InstalledBreakpoint> installedDeleter(installed);

	// If we still have enough hardware breakpoints left, install a hardware
	// breakpoint.
	Breakpoint* breakpoint = _GetUnusedHardwareBreakpoint(false);
	if (breakpoint != NULL) {
		status_t error = _InstallHardwareBreakpoint(breakpoint, address);
		if (error != B_OK)
			return error;

		breakpoint->installedBreakpoint = installed;
		installed->breakpoint = breakpoint;
	} else {
		// install a software breakpoint
		status_t error = _InstallSoftwareBreakpoint(installed, address);
		if (error != B_OK)
			return error;
	}

	fBreakpoints.Insert(installed);
	installedDeleter.Detach();
	fBreakpointCount++;

	return B_OK;
}


status_t
BreakpointManager::UninstallBreakpoint(void* _address)
{
	const addr_t address = (addr_t)_address;

	WriteLocker locker(fLock);

	InstalledBreakpoint* installed = fBreakpoints.Lookup(address);
	if (installed == NULL)
		return B_BAD_VALUE;

	if (installed->breakpoint->software)
		_UninstallSoftwareBreakpoint(installed->breakpoint);
	else
		_UninstallHardwareBreakpoint(installed->breakpoint);

	fBreakpoints.Remove(installed);
	delete installed;
	fBreakpointCount--;

	return B_OK;
}


status_t
BreakpointManager::InstallWatchpoint(void* _address, uint32 type, int32 length)
{
	const addr_t address = (addr_t)_address;

	WriteLocker locker(fLock);

	InstalledWatchpoint* watchpoint = _FindWatchpoint(address);
	if (watchpoint != NULL)
		return B_BAD_VALUE;

#if DEBUG_SHARED_BREAK_AND_WATCHPOINTS
	// We need at least one hardware breakpoint for our breakpoint management.
	if (fWatchpointCount + 1 >= DEBUG_MAX_WATCHPOINTS)
		return B_BUSY;
#else
	if (fWatchpointCount >= DEBUG_MAX_WATCHPOINTS)
		return B_BUSY;
#endif

	watchpoint = new(std::nothrow) InstalledWatchpoint;
	if (watchpoint == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<InstalledWatchpoint> watchpointDeleter(watchpoint);

	status_t error = _InstallWatchpoint(watchpoint, address, type, length);
	if (error != B_OK)
		return error;

	fWatchpoints.Add(watchpointDeleter.Detach());
	fWatchpointCount++;
	return B_OK;
}


status_t
BreakpointManager::UninstallWatchpoint(void* address)
{
	WriteLocker locker(fLock);

	InstalledWatchpoint* watchpoint = _FindWatchpoint((addr_t)address);
	if (watchpoint == NULL)
		return B_BAD_VALUE;

	ObjectDeleter<InstalledWatchpoint> deleter(watchpoint);
	fWatchpoints.Remove(watchpoint);
	fWatchpointCount--;

	return _UninstallWatchpoint(watchpoint);
}


void
BreakpointManager::RemoveAllBreakpoints()
{
	WriteLocker locker(fLock);

	// remove the breakpoints
	BreakpointTree::Iterator it = fBreakpoints.GetIterator();
	while (InstalledBreakpoint* installedBreakpoint = it.Next()) {
		it.Remove();

		// uninstall underlying hard/software breakpoint
		if (installedBreakpoint->breakpoint->software)
			_UninstallSoftwareBreakpoint(installedBreakpoint->breakpoint);
		else
			_UninstallHardwareBreakpoint(installedBreakpoint->breakpoint);

		delete installedBreakpoint;
	}

	// remove the watchpoints
	while (InstalledWatchpoint* watchpoint = fWatchpoints.RemoveHead()) {
		_UninstallWatchpoint(watchpoint);
		delete watchpoint;
	}
}


/*!	\brief Returns whether the given address can be accessed in principle.
	No check whether there's an actually accessible area is performed, though.
*/
/*static*/ bool
BreakpointManager::CanAccessAddress(const void* _address, bool write)
{
	const addr_t address = (addr_t)_address;

	// user addresses are always fine
	if (IS_USER_ADDRESS(address))
		return true;

	// a commpage address can at least be read
	if (address >= USER_COMMPAGE_ADDR
		&& address < USER_COMMPAGE_ADDR + COMMPAGE_SIZE) {
		return !write;
	}

	return false;
}


/*!	\brief Reads data from user memory.

	Tries to read \a size bytes of data from user memory address \a address
	into the supplied buffer \a buffer. If only a part could be read the
	function won't fail. The number of bytes actually read is return through
	\a bytesRead.

	\param address The user memory address from which to read.
	\param buffer The buffer into which to write.
	\param size The number of bytes to read.
	\param bytesRead Will be set to the number of bytes actually read.
	\return \c B_OK, if reading went fine. Then \a bytesRead will be set to
			the amount of data actually read. An error indicates that nothing
			has been read.
*/
status_t
BreakpointManager::ReadMemory(const void* _address, void* buffer, size_t size,
	size_t& bytesRead)
{
	const addr_t address = (addr_t)_address;

	ReadLocker locker(fLock);

	status_t error = _ReadMemory(address, buffer, size, bytesRead);
	if (error != B_OK)
		return error;

	// If we have software breakpoints installed, fix the buffer not to contain
	// any of them.

	// address of the first possibly intersecting software breakpoint
	const addr_t startAddress
		= std::max(address, (addr_t)DEBUG_SOFTWARE_BREAKPOINT_SIZE - 1)
			- (DEBUG_SOFTWARE_BREAKPOINT_SIZE - 1);

	BreakpointTree::Iterator it = fBreakpoints.GetIterator(startAddress, true,
		true);
	while (InstalledBreakpoint* installed = it.Next()) {
		Breakpoint* breakpoint = installed->breakpoint;
		if (breakpoint->address >= address + size)
			break;

		if (breakpoint->software) {
			// Software breakpoint intersects -- replace the read data with
			// the data saved in the breakpoint object.
			addr_t minAddress = std::max(breakpoint->address, address);
			size_t toCopy = std::min(address + size,
					breakpoint->address + DEBUG_SOFTWARE_BREAKPOINT_SIZE)
				- minAddress;
			memcpy((uint8*)buffer + (minAddress - address),
				breakpoint->softwareData + (minAddress - breakpoint->address),
				toCopy);
		}
	}

	return B_OK;
}


status_t
BreakpointManager::WriteMemory(void* _address, const void* _buffer, size_t size,
	size_t& bytesWritten)
{
	bytesWritten = 0;

	if (size == 0)
		return B_OK;

	addr_t address = (addr_t)_address;
	const uint8* buffer = (uint8*)_buffer;

	WriteLocker locker(fLock);

	// We don't want to overwrite software breakpoints, so things are a bit more
	// complicated. We iterate through the intersecting software breakpoints,
	// writing the memory between them normally, but skipping the breakpoints
	// itself. We write into their softwareData instead.

	// Get the first breakpoint -- if it starts before the address, we'll
	// handle it separately to make things in the main loop simpler.
	const addr_t startAddress
		= std::max(address, (addr_t)DEBUG_SOFTWARE_BREAKPOINT_SIZE - 1)
			- (DEBUG_SOFTWARE_BREAKPOINT_SIZE - 1);

	BreakpointTree::Iterator it = fBreakpoints.GetIterator(startAddress, true,
		true);
	InstalledBreakpoint* installed = it.Next();
	while (installed != NULL) {
		Breakpoint* breakpoint = installed->breakpoint;
		if (breakpoint->address >= address)
			break;

		if (breakpoint->software) {
			// We've got a breakpoint that is partially intersecting with the
			// beginning of the address range to write.
			size_t toCopy = std::min(address + size,
					breakpoint->address + DEBUG_SOFTWARE_BREAKPOINT_SIZE)
				- address;
			memcpy(breakpoint->softwareData + (address - breakpoint->address),
				buffer, toCopy);

			address += toCopy;
			size -= toCopy;
			bytesWritten += toCopy;
			buffer += toCopy;
		}

		installed = it.Next();
	}

	// loop through the breakpoints intersecting with the range
	while (installed != NULL) {
		Breakpoint* breakpoint = installed->breakpoint;
		if (breakpoint->address >= address + size)
			break;

		if (breakpoint->software) {
			// write the data up to the breakpoint (if any)
			size_t toCopy = breakpoint->address - address;
			if (toCopy > 0) {
				size_t chunkWritten;
				status_t error = _WriteMemory(address, buffer, toCopy,
					chunkWritten);
				if (error != B_OK)
					return bytesWritten > 0 ? B_OK : error;

				address += chunkWritten;
				size -= chunkWritten;
				bytesWritten += chunkWritten;
				buffer += chunkWritten;

				if (chunkWritten < toCopy)
					return B_OK;
			}

			// write to the breakpoint data
			toCopy = std::min(size, (size_t)DEBUG_SOFTWARE_BREAKPOINT_SIZE);
			memcpy(breakpoint->softwareData, buffer, toCopy);

			address += toCopy;
			size -= toCopy;
			bytesWritten += toCopy;
			buffer += toCopy;
		}

		installed = it.Next();
	}

	// write remaining data
	if (size > 0) {
		size_t chunkWritten;
		status_t error = _WriteMemory(address, buffer, size, chunkWritten);
		if (error != B_OK)
			return bytesWritten > 0 ? B_OK : error;

		bytesWritten += chunkWritten;
	}

	return B_OK;
}


void
BreakpointManager::PrepareToContinue(void* _address)
{
	const addr_t address = (addr_t)_address;

	WriteLocker locker(fLock);

	// Check whether there's a software breakpoint at the continuation address.
	InstalledBreakpoint* installed = fBreakpoints.Lookup(address);
	if (installed == NULL || !installed->breakpoint->software)
		return;

	// We need to replace the software breakpoint by a hardware one, or
	// we can't continue the thread.
	Breakpoint* breakpoint = _GetUnusedHardwareBreakpoint(true);
	if (breakpoint == NULL) {
		dprintf("Failed to allocate a hardware breakpoint.\n");
		return;
	}

	status_t error = _InstallHardwareBreakpoint(breakpoint, address);
	if (error != B_OK)
		return;

	_UninstallSoftwareBreakpoint(installed->breakpoint);

	breakpoint->installedBreakpoint = installed;
	installed->breakpoint = breakpoint;
}


BreakpointManager::Breakpoint*
BreakpointManager::_GetUnusedHardwareBreakpoint(bool force)
{
	// try to find a free one first
	for (BreakpointList::Iterator it = fHardwareBreakpoints.GetIterator();
			Breakpoint* breakpoint = it.Next();) {
		if (!breakpoint->used)
			return breakpoint;
	}

	if (!force)
		return NULL;

	// replace one by a software breakpoint
	for (BreakpointList::Iterator it = fHardwareBreakpoints.GetIterator();
			Breakpoint* breakpoint = it.Next();) {
		if (breakpoint->installedBreakpoint == NULL)
			continue;

		status_t error = _InstallSoftwareBreakpoint(
			breakpoint->installedBreakpoint, breakpoint->address);
		if (error != B_OK)
			continue;

		if (_UninstallHardwareBreakpoint(breakpoint) == B_OK)
			return breakpoint;
	}

	return NULL;
}


status_t
BreakpointManager::_InstallSoftwareBreakpoint(InstalledBreakpoint* installed,
	addr_t address)
{
	Breakpoint* breakpoint = new(std::nothrow) Breakpoint;
	if (breakpoint == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<Breakpoint> breakpointDeleter(breakpoint);

	breakpoint->address = address;
	breakpoint->installedBreakpoint = installed;
	breakpoint->used = true;
	breakpoint->software = true;

	// save the memory where the software breakpoint shall be installed
	size_t bytesTransferred;
	status_t error = _ReadMemory(address, breakpoint->softwareData,
		DEBUG_SOFTWARE_BREAKPOINT_SIZE, bytesTransferred);
	if (error != B_OK)
		return error;
	if (bytesTransferred != DEBUG_SOFTWARE_BREAKPOINT_SIZE)
		return B_BAD_ADDRESS;

	// write the breakpoint code
	error = _WriteMemory(address, DEBUG_SOFTWARE_BREAKPOINT,
		DEBUG_SOFTWARE_BREAKPOINT_SIZE, bytesTransferred);
	if (error != B_OK)
		return error;

	if (bytesTransferred < DEBUG_SOFTWARE_BREAKPOINT_SIZE) {
		// breakpoint written partially only -- undo the written part
		if (bytesTransferred > 0) {
			size_t dummy;
			_WriteMemory(address, breakpoint->softwareData, bytesTransferred,
				dummy);
		}
		return B_BAD_ADDRESS;
	}

	installed->breakpoint = breakpoint;
	breakpointDeleter.Detach();

	TRACE("installed software breakpoint at %#lx\n", address);

	return B_OK;
}


status_t
BreakpointManager::_UninstallSoftwareBreakpoint(Breakpoint* breakpoint)
{
	size_t bytesWritten;
	_WriteMemory(breakpoint->address, breakpoint->softwareData,
		DEBUG_SOFTWARE_BREAKPOINT_SIZE, bytesWritten);

	TRACE("uninstalled software breakpoint at %#lx\n", breakpoint->address);

	delete breakpoint;
	return B_OK;
}


status_t
BreakpointManager::_InstallHardwareBreakpoint(Breakpoint* breakpoint,
	addr_t address)
{
	status_t error = arch_set_breakpoint((void*)address);
	if (error != B_OK)
		return error;

	// move to the tail of the list
	fHardwareBreakpoints.Remove(breakpoint);
	fHardwareBreakpoints.Add(breakpoint);

	TRACE("installed hardware breakpoint at %#lx\n", address);

	breakpoint->address = address;
	breakpoint->used = true;
	return B_OK;
}


status_t
BreakpointManager::_UninstallHardwareBreakpoint(Breakpoint* breakpoint)
{
	status_t error = arch_clear_breakpoint((void*)breakpoint->address);
	if (error != B_OK)
		return error;

	TRACE("uninstalled hardware breakpoint at %#lx\n", breakpoint->address);

	breakpoint->used = false;
	breakpoint->installedBreakpoint = NULL;
	return B_OK;
}


BreakpointManager::InstalledWatchpoint*
BreakpointManager::_FindWatchpoint(addr_t address) const
{
	for (InstalledWatchpointList::ConstIterator it = fWatchpoints.GetIterator();
		InstalledWatchpoint* watchpoint = it.Next();) {
		if (address == watchpoint->address)
			return watchpoint;
	}

	return NULL;
}


status_t
BreakpointManager::_InstallWatchpoint(InstalledWatchpoint* watchpoint,
	addr_t address, uint32 type, int32 length)
{
#if DEBUG_SHARED_BREAK_AND_WATCHPOINTS
	// We need a hardware breakpoint.
	watchpoint->breakpoint = _GetUnusedHardwareBreakpoint(true);
	if (watchpoint->breakpoint == NULL) {
		dprintf("Failed to allocate a hardware breakpoint for watchpoint.\n");
		return B_BUSY;
	}
#endif

	status_t error = arch_set_watchpoint((void*)address, type, length);
	if (error != B_OK)
		return error;

	watchpoint->address = address;

#if DEBUG_SHARED_BREAK_AND_WATCHPOINTS
	watchpoint->breakpoint->used = true;
#endif

	return B_OK;
}


status_t
BreakpointManager::_UninstallWatchpoint(InstalledWatchpoint* watchpoint)
{
#if DEBUG_SHARED_BREAK_AND_WATCHPOINTS
	watchpoint->breakpoint->used = false;
#endif

	return arch_clear_watchpoint((void*)watchpoint->address);
}


status_t
BreakpointManager::_ReadMemory(const addr_t _address, void* _buffer,
	size_t size, size_t& bytesRead)
{
	const uint8* address = (const uint8*)_address;
	uint8* buffer = (uint8*)_buffer;

	// check the parameters
	if (!CanAccessAddress(address, false))
		return B_BAD_ADDRESS;
	if (size <= 0)
		return B_BAD_VALUE;

	// If the region to be read crosses page boundaries, we split it up into
	// smaller chunks.
	status_t error = B_OK;
	bytesRead = 0;
	while (size > 0) {
		// check whether we're still in user address space
		if (!CanAccessAddress(address, false)) {
			error = B_BAD_ADDRESS;
			break;
		}

		// don't cross page boundaries in a single read
		int32 toRead = size;
		int32 maxRead = B_PAGE_SIZE - (addr_t)address % B_PAGE_SIZE;
		if (toRead > maxRead)
			toRead = maxRead;

		error = user_memcpy(buffer, address, toRead);
		if (error != B_OK)
			break;

		bytesRead += toRead;
		address += toRead;
		buffer += toRead;
		size -= toRead;
	}

	// If reading fails, we only fail, if we haven't read anything yet.
	if (error != B_OK) {
		if (bytesRead > 0)
			return B_OK;
		return error;
	}

	return B_OK;
}


status_t
BreakpointManager::_WriteMemory(addr_t _address, const void* _buffer,
	size_t size, size_t& bytesWritten)
{
	uint8* address = (uint8*)_address;
	const uint8* buffer = (const uint8*)_buffer;

	// check the parameters
	if (!CanAccessAddress(address, true))
		return B_BAD_ADDRESS;
	if (size <= 0)
		return B_BAD_VALUE;

	// If the region to be written crosses area boundaries, we split it up into
	// smaller chunks.
	status_t error = B_OK;
	bytesWritten = 0;
	while (size > 0) {
		// check whether we're still in user address space
		if (!CanAccessAddress(address, true)) {
			error = B_BAD_ADDRESS;
			break;
		}

		// get the area for the address (we need to use _user_area_for(), since
		// we're looking for a user area)
		area_id area = _user_area_for(address);
		if (area < 0) {
			TRACE("BreakpointManager::_WriteMemory(): area not found for "
				"address: %p: %lx\n", address, area);
			error = area;
			break;
		}

		area_info areaInfo;
		status_t error = get_area_info(area, &areaInfo);
		if (error != B_OK) {
			TRACE("BreakpointManager::_WriteMemory(): failed to get info for "
				"area %ld: %lx\n", area, error);
			error = B_BAD_ADDRESS;
			break;
		}

		// restrict this round of writing to the found area
		int32 toWrite = size;
		int32 maxWrite = (uint8*)areaInfo.address + areaInfo.size - address;
		if (toWrite > maxWrite)
			toWrite = maxWrite;

		// if the area is read-only, we temporarily need to make it writable
		bool protectionChanged = false;
		if (!(areaInfo.protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA))) {
			error = set_area_protection(area,
				areaInfo.protection | B_WRITE_AREA);
			if (error != B_OK) {
				TRACE("BreakpointManager::_WriteMemory(): failed to set new "
					"protection for area %ld: %lx\n", area, error);
				break;
			}
			protectionChanged = true;
		}

		// copy the memory
		error = user_memcpy(address, buffer, toWrite);

		// reset the area protection
		if (protectionChanged)
			set_area_protection(area, areaInfo.protection);

		if (error != B_OK) {
			TRACE("BreakpointManager::_WriteMemory(): user_memcpy() failed: "
				"%lx\n", error);
			break;
		}

		bytesWritten += toWrite;
		address += toWrite;
		buffer += toWrite;
		size -= toWrite;
	}

	// If writing fails, we only fail, if we haven't written anything yet.
	if (error != B_OK) {
		if (bytesWritten > 0)
			return B_OK;
		return error;
	}

	return B_OK;
}
