/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "RemoteDebugRequest.h"

#include <stdlib.h>

#include <Message.h>

#include <debugger.h>

#include <AutoDeleter.h>

#include "Architecture.h"
#include "CpuState.h"


// #pragma mark - RemoteDebugRequest


RemoteDebugRequest::RemoteDebugRequest()
	:
	BReferenceable(),
	fArchitecture(NULL)
{
}


RemoteDebugRequest::~RemoteDebugRequest()
{
	if (fArchitecture != NULL)
		fArchitecture->ReleaseReference();
}


status_t
RemoteDebugRequest::LoadFromMessage(const BMessage& data)
{
	if (data.FindInt32("type") != Type())
		return B_BAD_VALUE;

	return LoadSpecificInfoFromMessage(data);
}


status_t
RemoteDebugRequest::SaveToMessage(BMessage& _output) const
{
	_output.MakeEmpty();

	status_t error = _output.AddInt32("type", Type());
	if (error != B_OK)
		return error;

	return SaveSpecificInfoToMessage(_output);
}


void
RemoteDebugRequest::SetArchitecture(Architecture* architecture)
{
	fArchitecture = architecture;
	fArchitecture->AcquireReference();
}


// #pragma mark - RemoteDebugResponse


RemoteDebugResponse::RemoteDebugResponse()
	:
	BReferenceable(),
	fRequest(NULL),
	fResult(B_OK)
{
}


RemoteDebugResponse::~RemoteDebugResponse()
{
	if (fRequest != NULL)
		fRequest->ReleaseReference();
}


void
RemoteDebugResponse::SetRequestInfo(RemoteDebugRequest* request,
	status_t result)
{
	fRequest = request;
	fRequest->AcquireReference();
	fResult = result;
}


status_t
RemoteDebugResponse::LoadFromMessage(const BMessage& data)
{
	if (data.FindInt32("type") != Request()->Type())
		return B_BAD_VALUE;

	if (!Succeeded())
		return B_OK;

	return LoadSpecificInfoFromMessage(data);
}


status_t
RemoteDebugResponse::SaveToMessage(BMessage& _output) const
{
	_output.MakeEmpty();

	status_t error = _output.AddInt32("type", Request()->Type());
	if (error != B_OK)
		return error;

	error = _output.AddInt32("result", Result());
	if (error != B_OK)
		return error;

	if (!Succeeded())
		return B_OK;

	return SaveSpecificInfoToMessage(_output);
}


status_t
RemoteDebugResponse::LoadSpecificInfoFromMessage(const BMessage& data)
{
	return B_OK;
}


status_t
RemoteDebugResponse::SaveSpecificInfoToMessage(BMessage& _output) const
{
	return B_OK;
}


// #pragma mark - RemoteDebugReadMemoryRequest


RemoteDebugReadMemoryRequest::RemoteDebugReadMemoryRequest()
	:
	RemoteDebugRequest(),
	fAddress(0),
	fSize(0)
{
}


RemoteDebugReadMemoryRequest::~RemoteDebugReadMemoryRequest()
{
}


void
RemoteDebugReadMemoryRequest::SetTo(target_addr_t address, target_size_t size)
{
	fAddress = address;
	fSize = size;
}


remote_request_type
RemoteDebugReadMemoryRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_READ_MEMORY;
}


status_t
RemoteDebugReadMemoryRequest::LoadSpecificInfoFromMessage(const BMessage& data)
{
	if (data.FindUInt64("address", &fAddress) != B_OK)
		return B_BAD_VALUE;

	if (data.FindUInt64("size", &fSize) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
RemoteDebugReadMemoryRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	status_t error = _output.AddUInt64("address", fAddress);
	if (error != B_OK)
		return error;

	return _output.AddUInt64("size", fSize);
}


// #pragma mark - RemoteDebugWriteMemoryRequest


RemoteDebugWriteMemoryRequest::RemoteDebugWriteMemoryRequest()
	:
	RemoteDebugRequest(),
	fAddress(0),
	fData(NULL),
	fSize(0)
{
}


RemoteDebugWriteMemoryRequest::~RemoteDebugWriteMemoryRequest()
{
	if (fData != NULL)
		free(fData);
}


status_t
RemoteDebugWriteMemoryRequest::SetTo(target_addr_t address, const void* data,
	target_size_t size)
{
	if (size == 0 || data == NULL)
		return B_BAD_VALUE;

	fAddress = address;
	fSize = size;
	fData = malloc(fSize);
	if (fData == NULL)
		return B_NO_MEMORY;


	memcpy(fData, data, fSize);
	return B_OK;
}


remote_request_type
RemoteDebugWriteMemoryRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_WRITE_MEMORY;
}


status_t
RemoteDebugWriteMemoryRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (data.FindUInt64("address", &fAddress) != B_OK)
		return B_BAD_VALUE;

	if (data.FindUInt64("size", &fSize) != B_OK)
		return B_BAD_VALUE;

	fData = malloc(fSize);
	if (fData == NULL)
		return B_NO_MEMORY;

	const void* messageData = NULL;
	ssize_t numBytes = -1;
	status_t error = data.FindData("data", B_RAW_TYPE, &messageData,
		&numBytes);
	if (error != B_OK)
		return error;

	if ((size_t)numBytes != fSize)
		return B_MISMATCHED_VALUES;

	memcpy(fData, messageData, numBytes);

	return B_OK;
}


status_t
RemoteDebugWriteMemoryRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	status_t error = _output.AddUInt64("address", fAddress);
	if (error != B_OK)
		return error;

	error = _output.AddUInt64("size", fSize);
	if (error != B_OK)
		return error;

	return _output.AddData("data", B_RAW_TYPE, fData, (ssize_t)fSize);
}


// #pragma mark - RemoteDebugSetTeamFlagsRequest


RemoteDebugSetTeamFlagsRequest::RemoteDebugSetTeamFlagsRequest()
	:
	RemoteDebugRequest(),
	fFlags(0)
{
}


RemoteDebugSetTeamFlagsRequest::~RemoteDebugSetTeamFlagsRequest()
{
}


void
RemoteDebugSetTeamFlagsRequest::SetTo(int32 flags)
{
	fFlags = flags;
}


remote_request_type
RemoteDebugSetTeamFlagsRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_SET_TEAM_FLAGS;
}


status_t
RemoteDebugSetTeamFlagsRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (data.FindInt32("flags", &fFlags) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
RemoteDebugSetTeamFlagsRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	return _output.AddInt32("flags", fFlags);
}


// #pragma mark - RemoteDebugSetThreadFlagsRequest


RemoteDebugSetThreadFlagsRequest::RemoteDebugSetThreadFlagsRequest()
	:
	RemoteDebugRequest(),
	fThread(-1),
	fFlags(0)
{
}


RemoteDebugSetThreadFlagsRequest::~RemoteDebugSetThreadFlagsRequest()
{
}


void
RemoteDebugSetThreadFlagsRequest::SetTo(thread_id thread, int32 flags)
{
	fThread = thread;
	fFlags = flags;
}


remote_request_type
RemoteDebugSetThreadFlagsRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_SET_THREAD_FLAGS;
}


status_t
RemoteDebugSetThreadFlagsRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (data.FindInt32("thread", &fThread) != B_OK)
		return B_BAD_VALUE;

	if (data.FindInt32("flags", &fFlags) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
RemoteDebugSetThreadFlagsRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	status_t error = _output.AddInt32("thread", fThread);
	if (error != B_OK)
		return error;

	return _output.AddInt32("flags", fFlags);
}


// #pragma mark - RemoteDebugThreadActionRequest


RemoteDebugThreadActionRequest::RemoteDebugThreadActionRequest()
	:
	RemoteDebugRequest(),
	fThread(-1)
{
}


RemoteDebugThreadActionRequest::~RemoteDebugThreadActionRequest()
{
}


void
RemoteDebugThreadActionRequest::SetTo(thread_id thread)
{
	fThread = thread;
}


status_t
RemoteDebugThreadActionRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (data.FindInt32("thread", &fThread) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
RemoteDebugThreadActionRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	return _output.AddInt32("thread", fThread);
}


// #pragma mark - RemoteDebugContinueThreadRequest


RemoteDebugContinueThreadRequest::RemoteDebugContinueThreadRequest()
	:
	RemoteDebugThreadActionRequest()
{
}


RemoteDebugContinueThreadRequest::~RemoteDebugContinueThreadRequest()
{
}

remote_request_type
RemoteDebugContinueThreadRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_CONTINUE_THREAD;
}


// #pragma mark - RemoteDebugStopThreadRequest


RemoteDebugStopThreadRequest::RemoteDebugStopThreadRequest()
	:
	RemoteDebugThreadActionRequest()
{
}


RemoteDebugStopThreadRequest::~RemoteDebugStopThreadRequest()
{
}

remote_request_type
RemoteDebugStopThreadRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_STOP_THREAD;
}


// #pragma mark - RemoteDebugSingleStepThreadRequest


RemoteDebugSingleStepThreadRequest::RemoteDebugSingleStepThreadRequest()
	:
	RemoteDebugThreadActionRequest()
{
}


RemoteDebugSingleStepThreadRequest::~RemoteDebugSingleStepThreadRequest()
{
}

remote_request_type
RemoteDebugSingleStepThreadRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_SINGLE_STEP_THREAD;
}


// #pragma mark - RemoteDebugGetCpuStateRequest


RemoteDebugGetCpuStateRequest::RemoteDebugGetCpuStateRequest()
	:
	RemoteDebugThreadActionRequest()
{
}


RemoteDebugGetCpuStateRequest::~RemoteDebugGetCpuStateRequest()
{
}


remote_request_type
RemoteDebugGetCpuStateRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_GET_CPU_STATE;
}


// #pragma mark - RemoteDebugSetCpuStateRequest


RemoteDebugSetCpuStateRequest::RemoteDebugSetCpuStateRequest()
	:
	RemoteDebugRequest(),
	fThread(-1),
	fCpuState(NULL)
{
}


RemoteDebugSetCpuStateRequest::~RemoteDebugSetCpuStateRequest()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}


void
RemoteDebugSetCpuStateRequest::SetTo(thread_id thread, CpuState* state)
{
	fThread = thread;
	fCpuState = state;
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


remote_request_type
RemoteDebugSetCpuStateRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_SET_CPU_STATE;
}


status_t
RemoteDebugSetCpuStateRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (data.FindInt32("thread", &fThread) != B_OK)
		return B_BAD_VALUE;

	if (fCpuState != NULL) {
		fCpuState->ReleaseReference();
		fCpuState = NULL;
	}

	const uint8* buffer = NULL;
	ssize_t numBytes = 0;
	size_t stateSize = GetArchitecture()->DebugCpuStateSize();
	status_t error = data.FindData("state", B_RAW_TYPE, (const void**)&buffer,
		&numBytes);
	if (error != B_OK || (size_t)numBytes != stateSize)
		return B_BAD_VALUE;

	return GetArchitecture()->CreateCpuState(buffer, stateSize, fCpuState);
}


status_t
RemoteDebugSetCpuStateRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	status_t error = _output.AddInt32("thread", fThread);
	if (error != B_OK)
		return error;

	size_t stateSize = GetArchitecture()->DebugCpuStateSize();
	uint8* buffer = new(std::nothrow) uint8[stateSize];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<uint8> deleter(buffer);
	error = fCpuState->UpdateDebugState(buffer, stateSize);
	if (error != B_OK)
		return error;

	return _output.AddData("state", B_RAW_TYPE, buffer, (ssize_t)stateSize);
}


// #pragma mark - RemoteDebugAddressActionRequest


RemoteDebugAddressActionRequest::RemoteDebugAddressActionRequest()
	:
	RemoteDebugRequest(),
	fAddress(0)
{
}


RemoteDebugAddressActionRequest::~RemoteDebugAddressActionRequest()
{
}


void
RemoteDebugAddressActionRequest::SetTo(target_addr_t address)
{
	fAddress = address;
}


status_t
RemoteDebugAddressActionRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	return data.FindUInt64("address", &fAddress);
}


status_t
RemoteDebugAddressActionRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	return _output.AddUInt64("address", fAddress);
}


// #pragma mark - RemoteDebugInstallBreakpointRequest


RemoteDebugInstallBreakpointRequest::RemoteDebugInstallBreakpointRequest()
	:
	RemoteDebugAddressActionRequest()
{
}


RemoteDebugInstallBreakpointRequest::~RemoteDebugInstallBreakpointRequest()
{
}


remote_request_type
RemoteDebugInstallBreakpointRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_INSTALL_BREAKPOINT;
}


// #pragma mark - RemoteDebugUninstallBreakpointRequest


RemoteDebugUninstallBreakpointRequest::RemoteDebugUninstallBreakpointRequest()
	:
	RemoteDebugAddressActionRequest()
{
}


RemoteDebugUninstallBreakpointRequest::~RemoteDebugUninstallBreakpointRequest()
{
}

remote_request_type
RemoteDebugUninstallBreakpointRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_UNINSTALL_BREAKPOINT;
}


// #pragma mark - RemoteDebugInstallWatchpointRequest


RemoteDebugInstallWatchpointRequest::RemoteDebugInstallWatchpointRequest()
	:
	RemoteDebugRequest(),
	fAddress(0),
	fWatchType(B_DATA_READ_WATCHPOINT),
	fLength(0)
{
}


RemoteDebugInstallWatchpointRequest::~RemoteDebugInstallWatchpointRequest()
{
}


void
RemoteDebugInstallWatchpointRequest::SetTo(target_addr_t address, uint32 type,
	int32 length)
{
	fAddress = address;
	fWatchType = type;
	fLength = length;
}


remote_request_type
RemoteDebugInstallWatchpointRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_INSTALL_WATCHPOINT;
}


status_t
RemoteDebugInstallWatchpointRequest::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	status_t error = data.FindUInt64("address", &fAddress);
	if (error != B_OK)
		return error;

	error = data.FindUInt32("watchtype", &fWatchType);
	if (error != B_OK)
		return error;

	return data.FindInt32("length", &fLength);
}


status_t
RemoteDebugInstallWatchpointRequest::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	status_t error = _output.AddUInt64("address", fAddress);
	if (error != B_OK)
		return error;

	error = _output.AddUInt32("watchtype", fWatchType);
	if (error != B_OK)
		return error;

	return _output.AddInt32("length", fLength);
}


// #pragma mark - RemoteDebugUninstallWatchpointRequest


RemoteDebugUninstallWatchpointRequest::RemoteDebugUninstallWatchpointRequest()
	:
	RemoteDebugAddressActionRequest()
{
}


RemoteDebugUninstallWatchpointRequest::~RemoteDebugUninstallWatchpointRequest()
{
}


remote_request_type
RemoteDebugUninstallWatchpointRequest::Type() const
{
	return REMOTE_REQUEST_TYPE_UNINSTALL_WATCHPOINT;
}


// #pragma mark - RemoteDebugReadMemoryResponse


RemoteDebugReadMemoryResponse::RemoteDebugReadMemoryResponse()
	:
	RemoteDebugResponse(),
	fData(NULL),
	fSize(0)
{
}


RemoteDebugReadMemoryResponse::~RemoteDebugReadMemoryResponse()
{
	if (fData != NULL)
		free(fData);
}


void
RemoteDebugReadMemoryResponse::SetTo(void* data, target_size_t size)
{
	fData = data;
	fSize = size;
}


status_t
RemoteDebugReadMemoryResponse::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	status_t error = data.FindUInt64("size", &fSize);
	if (error != B_OK)
		return error;

	fData = malloc(fSize);
	if (fData == NULL)
		return B_NO_MEMORY;

	const void* messageData = NULL;
	ssize_t numBytes = -1;
	error = data.FindData("data", B_RAW_TYPE, &messageData, &numBytes);
	if (error != B_OK)
		return error;

	if ((size_t)numBytes != fSize)
		return B_MISMATCHED_VALUES;

	memcpy(fData, messageData, numBytes);
	return B_OK;
}


status_t
RemoteDebugReadMemoryResponse::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	if (fData == NULL)
		return B_OK;

	status_t error = _output.AddUInt64("size", fSize);
	if (error != B_OK)
		return error;

	return _output.AddData("data", B_RAW_TYPE, fData, (ssize_t)fSize);
}


// #pragma mark - RemoteDebugGetCpuStateResponse


RemoteDebugGetCpuStateResponse::RemoteDebugGetCpuStateResponse()
	:
	RemoteDebugResponse(),
	fCpuState(NULL)
{
}


RemoteDebugGetCpuStateResponse::~RemoteDebugGetCpuStateResponse()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}


void
RemoteDebugGetCpuStateResponse::SetTo(CpuState* state)
{
	fCpuState = state;
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


status_t
RemoteDebugGetCpuStateResponse::LoadSpecificInfoFromMessage(
	const BMessage& data)
{
	if (fCpuState != NULL) {
		fCpuState->ReleaseReference();
		fCpuState = NULL;
	}

	const uint8* buffer = NULL;
	ssize_t numBytes = 0;
	size_t stateSize = GetArchitecture()->DebugCpuStateSize();
	status_t error = data.FindData("state", B_RAW_TYPE, (const void**)&buffer,
		&numBytes);
	if (error != B_OK || (size_t)numBytes != stateSize)
		return B_BAD_VALUE;

	return GetArchitecture()->CreateCpuState(buffer, stateSize, fCpuState);
}


status_t
RemoteDebugGetCpuStateResponse::SaveSpecificInfoToMessage(
	BMessage& _output) const
{
	size_t stateSize = GetArchitecture()->DebugCpuStateSize();
	uint8* buffer = new(std::nothrow) uint8[stateSize];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<uint8> deleter(buffer);
	status_t error = fCpuState->UpdateDebugState(buffer, stateSize);
	if (error != B_OK)
		return error;

	return _output.AddData("state", B_RAW_TYPE, buffer, (ssize_t)stateSize);
}
