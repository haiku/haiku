/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H


#include <OS.h>

#include <Referenceable.h>

#include "TeamMemoryBlock.h"
#include "Types.h"


class CpuState;
class FunctionInstance;
class Image;
class StackFrame;
class Team;
class Thread;
class TypeComponentPath;
class UserBreakpoint;
class UserInterfaceListener;
class ValueNode;
class ValueNodeContainer;
class Variable;


enum user_notification_type {
	USER_NOTIFICATION_INFO,
	USER_NOTIFICATION_WARNING,
	USER_NOTIFICATION_ERROR
};


class UserInterface : public BReferenceable {
public:
	virtual						~UserInterface();

	virtual	status_t			Init(Team* team,
									UserInterfaceListener* listener) = 0;
	virtual	void				Show() = 0;
	virtual	void				Terminate() = 0;
									// shut down the UI *now* -- no more user
									// feedback

	virtual	void				NotifyUser(const char* title,
									const char* message,
									user_notification_type type) = 0;
	virtual	int32				SynchronouslyAskUser(const char* title,
									const char* message, const char* choice1,
									const char* choice2, const char* choice3)
									= 0;
};


class UserInterfaceListener {
public:
	virtual						~UserInterfaceListener();

	virtual	void				FunctionSourceCodeRequested(
									FunctionInstance* function) = 0;
	virtual void				SourceEntryLocateRequested(
									const char* sourcePath,
									const char* locatedPath) = 0;
	virtual	void				ImageDebugInfoRequested(Image* image) = 0;
	virtual	void				ValueNodeValueRequested(CpuState* cpuState,
									ValueNodeContainer* container,
									ValueNode* valueNode) = 0;
	virtual	void				ThreadActionRequested(thread_id threadID,
									uint32 action) = 0;

	virtual	void				SetBreakpointRequested(target_addr_t address,
									bool enabled) = 0;
	virtual	void				SetBreakpointEnabledRequested(
									UserBreakpoint* breakpoint,
									bool enabled) = 0;
	virtual	void				ClearBreakpointRequested(
									target_addr_t address) = 0;
	virtual	void				ClearBreakpointRequested(
									UserBreakpoint* breakpoint) = 0;
									// TODO: Consolidate those!

	virtual void				InspectRequested(
									target_addr_t address,
									TeamMemoryBlock::Listener* listener) = 0;

	virtual	bool				UserInterfaceQuitRequested() = 0;
};


#endif	// USER_INTERFACE_H
