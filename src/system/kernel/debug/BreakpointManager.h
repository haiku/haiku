/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_MANAGER_H
#define BREAKPOINT_MANAGER_H

#include <util/DoublyLinkedList.h>
#include <util/SplayTree.h>

#include <arch/user_debugger.h>
#include <lock.h>


struct BreakpointManager {
public:
								BreakpointManager();
								~BreakpointManager();

			status_t			Init();

			status_t			InstallBreakpoint(void* address);
			status_t			UninstallBreakpoint(void* address);

			status_t			InstallWatchpoint(void* address, uint32 type,
									int32 length);
			status_t			UninstallWatchpoint(void* address);

			void				RemoveAllBreakpoints();
									// break- and watchpoints

	static	bool				CanAccessAddress(const void* address,
									bool write);
			status_t			ReadMemory(const void* _address, void* _buffer,
									size_t size, size_t& bytesRead);
			status_t			WriteMemory(void* _address, const void* _buffer,
									size_t size, size_t& bytesWritten);

			void				PrepareToContinue(void* address);

private:
			struct InstalledBreakpoint;

			struct Breakpoint : DoublyLinkedListLinkImpl<Breakpoint> {
				addr_t					address;
				InstalledBreakpoint*	installedBreakpoint;
				bool					used;
				bool					software;
				uint8					softwareData[
											DEBUG_SOFTWARE_BREAKPOINT_SIZE];
			};

			typedef DoublyLinkedList<Breakpoint> BreakpointList;

			struct InstalledBreakpoint : SplayTreeLink<InstalledBreakpoint> {
				InstalledBreakpoint*	splayNext;
				Breakpoint*				breakpoint;
				addr_t					address;

				InstalledBreakpoint(addr_t address);
			};

			struct InstalledWatchpoint
					: DoublyLinkedListLinkImpl<InstalledWatchpoint> {
				addr_t					address;
#if DEBUG_SHARED_BREAK_AND_WATCHPOINTS
				Breakpoint*				breakpoint;
#endif
			};

			typedef DoublyLinkedList<InstalledWatchpoint>
				InstalledWatchpointList;

			struct InstalledBreakpointSplayDefinition {
				typedef addr_t				KeyType;
				typedef	InstalledBreakpoint	NodeType;

				static const KeyType& GetKey(const InstalledBreakpoint* node)
				{
					return node->address;
				}

				static SplayTreeLink<NodeType>* GetLink(
					InstalledBreakpoint* node)
				{
					return node;
				}

				static int Compare(addr_t key, const InstalledBreakpoint* node)
				{
					if (key < node->address)
						return -1;
					return key == node->address ? 0 : 1;
				}

				// for IteratableSplayTree only
				static NodeType** GetListLink(InstalledBreakpoint* node)
				{
					return &node->splayNext;
				}
			};

			typedef IteratableSplayTree<InstalledBreakpointSplayDefinition>
				BreakpointTree;

private:
			Breakpoint*			_GetUnusedHardwareBreakpoint(bool force);

			status_t			_InstallSoftwareBreakpoint(
									InstalledBreakpoint* installed,
									addr_t address);
			status_t			_UninstallSoftwareBreakpoint(
									Breakpoint* breakpoint);

			status_t			_InstallHardwareBreakpoint(
									Breakpoint* breakpoint, addr_t address);
			status_t			_UninstallHardwareBreakpoint(
									Breakpoint* breakpoint);

			InstalledWatchpoint* _FindWatchpoint(addr_t address) const;
			status_t			_InstallWatchpoint(
									InstalledWatchpoint* watchpoint,
									addr_t address, uint32 type, int32 length);
			status_t			_UninstallWatchpoint(
									InstalledWatchpoint* watchpoint);

			status_t			_ReadMemory(const addr_t _address,
									void* _buffer, size_t size,
									size_t& bytesRead);
			status_t			_WriteMemory(addr_t _address,
									const void* _buffer, size_t size,
									size_t& bytesWritten);

private:
			rw_lock				fLock;
			BreakpointList		fHardwareBreakpoints;
			BreakpointTree		fBreakpoints;
			InstalledWatchpointList fWatchpoints;
			int32				fBreakpointCount;
			int32				fWatchpointCount;
};


#endif	// BREAKPOINT_MANAGER_H
