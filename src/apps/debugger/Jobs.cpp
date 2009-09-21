/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Architecture.h"
#include "BitBuffer.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "DisassembledCode.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "Register.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackFrameDebugInfo.h"
#include "StackFrameValueInfos.h"
#include "StackFrameValues.h"
#include "StackTrace.h"
#include "Team.h"
#include "TeamDebugInfo.h"
#include "Thread.h"
#include "Tracing.h"
#include "Type.h"
#include "TypeComponentPath.h"
#include "ValueLocation.h"
#include "Variable.h"


// #pragma mark - GetThreadStateJob


GetThreadStateJob::GetThreadStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_THREAD_STATE),
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AcquireReference();
}


GetThreadStateJob::~GetThreadStateJob()
{
	fThread->ReleaseReference();
}


const JobKey&
GetThreadStateJob::Key() const
{
	return fKey;
}


status_t
GetThreadStateJob::Do()
{
	CpuState* state = NULL;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	Reference<CpuState> reference(state, true);

	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() != THREAD_STATE_UNKNOWN)
		return B_OK;

	if (error == B_OK) {
		fThread->SetState(THREAD_STATE_STOPPED);
		fThread->SetCpuState(state);
	} else if (error == B_BAD_THREAD_STATE) {
		fThread->SetState(THREAD_STATE_RUNNING);
	} else
		return error;

	return B_OK;
}


// #pragma mark - GetCpuStateJob


GetCpuStateJob::GetCpuStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_CPU_STATE),
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AcquireReference();
}


GetCpuStateJob::~GetCpuStateJob()
{
	fThread->ReleaseReference();
}


const JobKey&
GetCpuStateJob::Key() const
{
	return fKey;
}


status_t
GetCpuStateJob::Do()
{
	CpuState* state;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	if (error != B_OK)
		return error;
	Reference<CpuState> reference(state, true);

	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() == THREAD_STATE_STOPPED)
		fThread->SetCpuState(state);

	return B_OK;
}


// #pragma mark - GetStackTraceJob


GetStackTraceJob::GetStackTraceJob(DebuggerInterface* debuggerInterface,
	Architecture* architecture, Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_STACK_TRACE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fThread(thread)
{
	fThread->AcquireReference();

	fCpuState = fThread->GetCpuState();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


GetStackTraceJob::~GetStackTraceJob()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();

	fThread->ReleaseReference();
}


const JobKey&
GetStackTraceJob::Key() const
{
	return fKey;
}


status_t
GetStackTraceJob::Do()
{
	if (fCpuState == NULL)
		return B_BAD_VALUE;

	// get the stack trace
	StackTrace* stackTrace;
	status_t error = fArchitecture->CreateStackTrace(fThread->GetTeam(), this,
		fCpuState, stackTrace);
	if (error != B_OK)
		return error;
	Reference<StackTrace> stackTraceReference(stackTrace, true);

	// set the stack trace, unless something has changed
	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->GetCpuState() == fCpuState)
		fThread->SetStackTrace(stackTrace);

	return B_OK;
}


status_t
GetStackTraceJob::GetImageDebugInfo(Image* image, ImageDebugInfo*& _info)
{
	AutoLocker<Team> teamLocker(fThread->GetTeam());

	while (image->GetImageDebugInfo() == NULL) {
		// schedule a job, if not loaded
		ImageDebugInfo* info;
		status_t error = LoadImageDebugInfoJob::ScheduleIfNecessary(GetWorker(),
			image, &info);
		if (error != B_OK)
			return error;

		if (info != NULL) {
			_info = info;
			return B_OK;
		}

		teamLocker.Unlock();

		// wait for the job to finish
		switch (WaitFor(SimpleJobKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO))) {
			case JOB_DEPENDENCY_SUCCEEDED:
			case JOB_DEPENDENCY_NOT_FOUND:
				// "Not found" can happen due to a race condition between
				// unlocking the worker and starting to wait.
				break;
			case JOB_DEPENDENCY_FAILED:
			case JOB_DEPENDENCY_ABORTED:
			default:
				return B_ERROR;
		}

		teamLocker.Lock();
	}

	_info = image->GetImageDebugInfo();
	_info->AcquireReference();

	return B_OK;
}


// #pragma mark - LoadImageDebugInfoJob


LoadImageDebugInfoJob::LoadImageDebugInfoJob(Image* image)
	:
	fKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO),
	fImage(image)
{
	fImage->AcquireReference();
}


LoadImageDebugInfoJob::~LoadImageDebugInfoJob()
{
	fImage->ReleaseReference();
}


const JobKey&
LoadImageDebugInfoJob::Key() const
{
	return fKey;
}


status_t
LoadImageDebugInfoJob::Do()
{
	// get an image info for the image
	AutoLocker<Team> locker(fImage->GetTeam());
	ImageInfo imageInfo(fImage->Info());
	locker.Unlock();

	// create the debug info
	ImageDebugInfo* debugInfo;
	status_t error = fImage->GetTeam()->DebugInfo()->LoadImageDebugInfo(
		imageInfo, fImage->ImageFile(), debugInfo);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		error = fImage->SetImageDebugInfo(debugInfo, IMAGE_DEBUG_INFO_LOADED);
		debugInfo->ReleaseReference();
	} else {
		fImage->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);
		delete debugInfo;
	}

	return error;
}


/*static*/ status_t
LoadImageDebugInfoJob::ScheduleIfNecessary(Worker* worker, Image* image,
	ImageDebugInfo** _imageDebugInfo)
{
	AutoLocker<Team> teamLocker(image->GetTeam());

	// If already loaded, we're done.
	if (image->GetImageDebugInfo() != NULL) {
		if (_imageDebugInfo != NULL) {
			*_imageDebugInfo = image->GetImageDebugInfo();
			(*_imageDebugInfo)->AcquireReference();
		}
		return B_OK;
	}

	// If already loading, the caller has to wait, if desired.
	if (image->ImageDebugInfoState() == IMAGE_DEBUG_INFO_LOADING) {
		if (_imageDebugInfo != NULL)
			*_imageDebugInfo = NULL;
		return B_OK;
	}

	// If an earlier load attempt failed, bail out.
	if (image->ImageDebugInfoState() != IMAGE_DEBUG_INFO_NOT_LOADED)
		return B_ERROR;

	// schedule a job
	LoadImageDebugInfoJob* job = new(std::nothrow) LoadImageDebugInfoJob(image);
	if (job == NULL)
		return B_NO_MEMORY;

	status_t error = worker->ScheduleJob(job);
	if (error != B_OK) {
		image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);
		return error;
	}

	image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_LOADING);

	if (_imageDebugInfo != NULL)
		*_imageDebugInfo = NULL;
	return B_OK;
}


// #pragma mark - LoadSourceCodeJob


LoadSourceCodeJob::LoadSourceCodeJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	Team* team, FunctionInstance* functionInstance, bool loadForFunction)
	:
	fKey(functionInstance, JOB_TYPE_LOAD_SOURCE_CODE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fTeam(team),
	fFunctionInstance(functionInstance),
	fLoadForFunction(loadForFunction)
{
	fFunctionInstance->AcquireReference();
}


LoadSourceCodeJob::~LoadSourceCodeJob()
{
	fFunctionInstance->ReleaseReference();
}


const JobKey&
LoadSourceCodeJob::Key() const
{
	return fKey;
}


status_t
LoadSourceCodeJob::Do()
{
	// if requested, try loading the source code for the function
	Function* function = fFunctionInstance->GetFunction();
	if (fLoadForFunction) {
		FileSourceCode* sourceCode;
		status_t error = fTeam->DebugInfo()->LoadSourceCode(
			function->SourceFile(), sourceCode);

		AutoLocker<Team> locker(fTeam);

		if (error == B_OK) {
			function->SetSourceCode(sourceCode, FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
			return B_OK;
		}

		function->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);
	}

	// Only try to load the function instance code, if it's not overridden yet.
	AutoLocker<Team> locker(fTeam);
	if (fFunctionInstance->SourceCodeState() != FUNCTION_SOURCE_LOADING)
		return B_OK;
	locker.Unlock();

	// disassemble the function
	DisassembledCode* sourceCode = NULL;
	status_t error = fTeam->DebugInfo()->DisassembleFunction(fFunctionInstance,
		sourceCode);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		if (fFunctionInstance->SourceCodeState() == FUNCTION_SOURCE_LOADING) {
			fFunctionInstance->SetSourceCode(sourceCode,
				FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
		}
	} else
		fFunctionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);

	return error;
}


// #pragma mark - GetStackFrameValueJobKey


GetStackFrameValueJobKey::GetStackFrameValueJobKey(StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
	:
	stackFrame(stackFrame),
	variable(variable),
	path(path)
{
}


uint32
GetStackFrameValueJobKey::HashValue() const
{
	uint32 hash = (uint32)(addr_t)stackFrame;
	hash = hash * 13 + (uint32)(addr_t)variable;
	return hash * 13 + path->HashValue();
}


bool
GetStackFrameValueJobKey::operator==(const JobKey& other) const
{
	const GetStackFrameValueJobKey* otherKey
		= dynamic_cast<const GetStackFrameValueJobKey*>(&other);
	return otherKey != NULL && stackFrame == otherKey->stackFrame
		&& variable == otherKey->variable && *path == *otherKey->path;
}


// #pragma mark - GetStackFrameValueJob


GetStackFrameValueJob::GetStackFrameValueJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	Thread* thread, StackFrame* stackFrame, Variable* variable,
	TypeComponentPath* path)
	:
	fKey(stackFrame, variable, path),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fThread(thread),
	fStackFrame(stackFrame),
	fVariable(variable),
	fPath(path)
{
	fThread->AcquireReference();
	fStackFrame->AcquireReference();
	fVariable->AcquireReference();
	fPath->AcquireReference();
}


GetStackFrameValueJob::~GetStackFrameValueJob()
{
	fThread->ReleaseReference();
	fStackFrame->ReleaseReference();
	fVariable->ReleaseReference();
	fPath->ReleaseReference();
}


const JobKey&
GetStackFrameValueJob::Key() const
{
	return fKey;
}


status_t
GetStackFrameValueJob::Do()
{
	status_t error = _GetValue();
	if (error == B_OK)
		return B_OK;

	// in case of error, set the value to invalid to avoid triggering this job
	// again
	AutoLocker<Team> locker(fThread->GetTeam());
	fStackFrame->Values()->SetValue(fVariable->ID(), fPath, BVariant());

	return error;
}


status_t
GetStackFrameValueJob::_GetValue()
{
	TRACE_LOCALS_ONLY(
		TRACE_LOCALS("GetStackFrameValueJob::_GetValue(): %s ",
			fVariable->Name().String());
		fPath->Dump();
		TRACE_LOCALS("\n");
	)

	Type* type;
	ValueLocation* location;
	bool valueResolved;
	status_t error = _ResolveTypeAndLocation(type, location, valueResolved);
	if (error != B_OK || valueResolved) {
		TRACE_LOCALS("  -> error: %#lx, valueResolved: %d\n", error,
			valueResolved);
		return error;
	}
	Type* actualType = type;
	Reference<Type> typeReference(type);
	Reference<Type> actualTypeReference(actualType);
	Reference<ValueLocation> locationReference(location);

	// find out the type of the data we want to read
	type_code valueType = 0;
	while (valueType == 0) {
		switch (type->Kind()) {
			case TYPE_PRIMITIVE:
				valueType = dynamic_cast<PrimitiveType*>(type)->TypeConstant();

				TRACE_LOCALS("  TYPE_PRIMITIVE: '%c%c%c%c'\n",
					int(valueType >> 24), int(valueType >> 16),
					int(valueType >> 8), int(valueType));

				if (valueType == 0) {
					TRACE_LOCALS("  -> unknown type constant\n");
					return B_BAD_VALUE;
				}
				break;
			case TYPE_MODIFIED:
				TRACE_LOCALS("  TYPE_MODIFIED\n");
				// ignore modifiers
				type = dynamic_cast<ModifiedType*>(type)->BaseType();
				break;
			case TYPE_TYPEDEF:
				TRACE_LOCALS("  TYPE_TYPEDEF\n");
				type = dynamic_cast<TypedefType*>(type)->BaseType();
				break;
			case TYPE_ADDRESS:
				TRACE_LOCALS("  TYPE_ADDRESS\n");
				if (fArchitecture->AddressSize() == 4) {
					valueType = B_UINT32_TYPE;
					TRACE_LOCALS("    -> 32 bit\n");
				} else {
					valueType = B_UINT64_TYPE;
					TRACE_LOCALS("    -> 64 bit\n");
				}
				break;
			case TYPE_COMPOUND:
			case TYPE_ARRAY:
				TRACE_LOCALS("  TYPE_COMPOUND/TYPE_ARRAY\n");
				// We can't retrieve the actual value of the compound object/
				// array (just of its components/elements), but to make the
				// recursion work smoothly, we have to set the type and
				// location at least.
				return _SetValue(BVariant(), actualType, location);
			default:
				TRACE_LOCALS("  default -> unsupported\n");
				return B_UNSUPPORTED;
		}
	}

	// update the reference in case the type has changed
	typeReference.SetTo(type);

	if (valueType == B_STRING_TYPE) {
		TRACE_LOCALS("  -> B_STRING_TYPE: unsupported\n");
		return B_UNSUPPORTED;
			// TODO:...
	}

	// check whether we know the complete location
	int32 count = location->CountPieces();

	TRACE_LOCALS("  location: %p, %ld pieces\n", location, count);

	if (count == 0) {
		TRACE_LOCALS("  -> no location\n");
		return B_ENTRY_NOT_FOUND;
	}

	// If the source language implementation uses descriptors to point to
	// objects, we need to resolve the object address to the data address.
	if (count == 1) {
		ValuePieceLocation piece = location->PieceAt(0);
		if (piece.type == VALUE_PIECE_LOCATION_MEMORY) {
			ValueLocation* dataLocation;
			error = fStackFrame->DebugInfo()->ResolveObjectDataLocation(
				fStackFrame, type, piece.address, dataLocation);
			if (error != B_OK)
				return error;

			location = dataLocation;
			locationReference.SetTo(location, true);
		}
	}

	target_size_t totalSize = 0;
	uint64 totalBitSize = 0;
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = location->PieceAt(i);
		switch (piece.type) {
			case VALUE_PIECE_LOCATION_INVALID:
			case VALUE_PIECE_LOCATION_UNKNOWN:
				return B_ENTRY_NOT_FOUND;
			case VALUE_PIECE_LOCATION_MEMORY:
			case VALUE_PIECE_LOCATION_REGISTER:
				break;
		}

		totalSize += piece.size;
		totalBitSize += piece.bitSize;
	}

	TRACE_LOCALS("  -> totalSize: %llu, totalBitSize: %llu\n", totalSize,
		totalBitSize);

	if (totalSize == 0 && totalBitSize == 0) {
		TRACE_LOCALS("  -> no size\n");
		return B_ENTRY_NOT_FOUND;
	}

	if (totalSize > 8 || totalSize + (totalBitSize + 7) / 8 > 8) {
		TRACE_LOCALS("  -> longer than 8 bytes: unsupported\n");
		return B_UNSUPPORTED;
	}

	if (totalSize + (totalBitSize + 7) / 8 < BVariant::SizeOfType(valueType)) {
		TRACE_LOCALS("  -> too short for value type (%llu vs. %lu)\n",
			totalSize + (totalBitSize + 7) / 8,
			BVariant::SizeOfType(valueType));
		return B_BAD_VALUE;
	}

	// load the data
	BitBuffer valueBuffer;

	// If the total bit size is not byte aligned, push the respective number of
	// 0 bits on a big endian architecture to get an immediately usable value.
	// On a little endian architecture we'll play with the last read byte
	// instead.
	if (fArchitecture->IsBigEndian() && totalBitSize % 8 != 0) {
		const uint8 zero = 0;
		valueBuffer.AddBits(&zero, 8 - totalBitSize % 8, 0);
	}

	const Register* registers = fArchitecture->Registers();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = location->PieceAt(i);
		uint8 bitOffset = piece.bitOffset;
		uint32 bitSize = piece.size * 8 + piece.bitSize;
		uint32 bytesToRead = (bitSize + 7) / 8;

		switch (piece.type) {
			case VALUE_PIECE_LOCATION_INVALID:
			case VALUE_PIECE_LOCATION_UNKNOWN:
				return B_ENTRY_NOT_FOUND;
			case VALUE_PIECE_LOCATION_MEMORY:
			{
				target_addr_t address = piece.address + bitOffset / 8;

				TRACE_LOCALS("  piece %ld: memory address: %#llx, bits: %lu\n",
					i, address, bitSize);

				bitOffset %= 8;
				uint8 pieceBuffer[8];
				ssize_t bytesRead = fDebuggerInterface->ReadMemory(address,
					pieceBuffer, bytesToRead);
				if (bytesRead < 0)
					return bytesRead;
				if ((uint32)bytesRead != bytesToRead)
					return B_BAD_ADDRESS;

				TRACE_LOCALS_ONLY(
					TRACE_LOCALS("  -> read: ");
					for (ssize_t k = 0; k < bytesRead; k++)
						TRACE_LOCALS("%02x", pieceBuffer[k]);
					TRACE_LOCALS("\n");
				)

				valueBuffer.AddBits(pieceBuffer, bitSize, bitOffset);
				break;
			}
			case VALUE_PIECE_LOCATION_REGISTER:
			{
				TRACE_LOCALS("  piece %ld: register: %lu, bits: %lu\n", i,
					piece.reg, bitSize);

				BVariant registerValue;
				if (!fStackFrame->GetCpuState()->GetRegisterValue(
						registers + piece.reg, registerValue)) {
					return B_ENTRY_NOT_FOUND;
				}
				if (registerValue.Size() < bytesToRead)
					return B_ENTRY_NOT_FOUND;

				if (!fArchitecture->IsHostEndian())
					registerValue.SwapEndianess();
				valueBuffer.AddBits(registerValue.Bytes(), bitSize, bitOffset);
				break;
			}
		}
	}

	// If the total bit size is not byte aligned, shift the last byte by the
	// respective number of bits on a little endian architecture to get a usable
	// value.
	if (!fArchitecture->IsBigEndian() && totalBitSize % 8 != 0)
		valueBuffer.Bytes()[totalBitSize / 8] >>= 8 - totalBitSize % 8;
			// TODO: Verify that this is the way to handle it!

	// convert the bits into something we can work with
	BVariant value;
	error = value.SetToTypedData(valueBuffer.Bytes(), valueType);
	if (error != B_OK) {
		TRACE_LOCALS("  -> failed to set typed data: %s\n", strerror(error));
		return error;
	}

	if (!fArchitecture->IsHostEndian())
		value.SwapEndianess();

	return _SetValue(value, actualType, location);
}


status_t
GetStackFrameValueJob::_SetValue(const BVariant& value, Type* type,
	ValueLocation* location)
{
	// set the value
	AutoLocker<Team> locker(fThread->GetTeam());

	status_t error = fStackFrame->Values()->SetValue(fVariable->ID(), fPath,
		value);
	if (error != B_OK) {
		TRACE_LOCALS("  -> failed to set value: %s\n", strerror(error));
		return error;
	}

	fStackFrame->ValueInfos()->SetInfo(fVariable->ID(), fPath, type, location);

	fStackFrame->NotifyValueRetrieved(fVariable, fPath);
	return B_OK;
}


status_t
GetStackFrameValueJob::_ResolveTypeAndLocation(Type*& _type,
	ValueLocation*& _location, bool& _valueResolved)
{
	if (fPath->CountComponents() == 0) {
		fVariable->GetType()->AcquireReference();
		fVariable->Location()->AcquireReference();
		_type = fVariable->GetType();
		_location = fVariable->Location();
		_valueResolved = false;
		return B_OK;
	}

	// get the parent value
	int32 componentCount = fPath->CountComponents();
	TypeComponentPath* parentPath = fPath->CreateSubPath(componentCount - 1);
	if (parentPath == NULL)
		return B_NO_MEMORY;
	Reference<TypeComponentPath> parentPathReference(parentPath, true);

	Type* parentType;
	ValueLocation* parentLocation;
	BVariant parentValue;
	status_t error = _GetTypeLocationAndValue(parentPath, parentType,
		parentLocation, parentValue);
	if (error != B_OK) {
		TRACE_LOCALS("GetStackFrameValueJob::_ResolveTypeAndLocation(): "
			"_GetTypeLocationAndValue() failed: %s\n", strerror(error));
		return error;
	}
	Reference<Type> parentTypeReference(parentType, true);
	Reference<ValueLocation> parentLocationReference(parentLocation, true);

	// resolve the last component
	TypeComponent component = fPath->ComponentAt(componentCount - 1);
	switch (component.typeKind) {
		case TYPE_PRIMITIVE:
			// cannot happen
			TRACE_LOCALS("GetStackFrameValueJob::_ResolveTypeAndLocation(): "
				"TYPE_PRIMITIVE subcomponent!\n");
			return B_BAD_VALUE;
		case TYPE_COMPOUND:
		{
			CompoundType* compoundType
				= dynamic_cast<CompoundType*>(parentType);

			// base type
			if (component.componentKind == TYPE_COMPONENT_BASE_TYPE) {
				BaseType* baseType = compoundType->BaseTypeAt(
					component.index);
				if (baseType == NULL)
					return B_BAD_VALUE;

				// The parent's location refers to the location of the complete
				// object. We want to extract the location of a member.
				ValueLocation* location;
				error = fStackFrame->DebugInfo()->ResolveBaseTypeLocation(
					fStackFrame, parentType, baseType, *parentLocation,
					location);
				if (error != B_OK) {
					TRACE_LOCALS("GetStackFrameValueJob::"
						"_ResolveTypeAndLocation(): TYPE_COMPOUND: "
						"ResolveBaseTypeLocation() failed: %s\n",
						strerror(error));
					return error;
				}

				baseType->GetType()->AcquireReference();
				_type = baseType->GetType();
				_location = location;
				_valueResolved = false;

				return B_OK;
			}

			// data member
			if (component.componentKind == TYPE_COMPONENT_DATA_MEMBER) {
				DataMember* dataMember = compoundType->DataMemberAt(
					component.index);
				if (dataMember == NULL)
					return B_BAD_VALUE;

				// The parent's location refers to the location of the complete
				// object. We want to extract the location of a member.
				ValueLocation* location;
				error = fStackFrame->DebugInfo()->ResolveDataMemberLocation(
					fStackFrame, parentType, dataMember, *parentLocation,
					location);
				if (error != B_OK) {
					TRACE_LOCALS("GetStackFrameValueJob::"
						"_ResolveTypeAndLocation(): TYPE_COMPOUND: "
						"ResolveDataMemberLocation() failed: %s\n",
						strerror(error));
					return error;
				}

				dataMember->GetType()->AcquireReference();
				_type = dataMember->GetType();
				_location = location;
				_valueResolved = false;

				return B_OK;
			}

			return B_UNSUPPORTED;
		}
		case TYPE_MODIFIED:
		case TYPE_TYPEDEF:
		{
			Type* type = component.typeKind == TYPE_MODIFIED
				? dynamic_cast<ModifiedType*>(parentType)->BaseType()
				: dynamic_cast<TypedefType*>(parentType)->BaseType();
			_valueResolved = true;
			return _SetValue(parentValue, type, parentLocation);
		}
		case TYPE_ADDRESS:
		{
			// The parent's value is an address pointing to this component.
			// resolve the location
			Type* type = dynamic_cast<AddressType*>(parentType)->BaseType();
			ValueLocation* location;
			error = fStackFrame->DebugInfo()->ResolveObjectDataLocation(
				fStackFrame, type, parentValue.ToUInt64(), location);
			if (error != B_OK) {
				TRACE_LOCALS("GetStackFrameValueJob::"
					"_ResolveTypeAndLocation(): TYPE_ADDRESS: "
					"ResolveObjectDataLocation() failed: %s\n",
					strerror(error));
				return error;
			}

			type->AcquireReference();
			_type = type;
			_location = location;
			_valueResolved = false;

			return B_OK;
		}
		case TYPE_ARRAY:
			// TODO:...
		default:
			return B_UNSUPPORTED;
	}
}


status_t
GetStackFrameValueJob::_GetTypeLocationAndValue(TypeComponentPath* parentPath,
	Type*& _parentType, ValueLocation*& _parentLocation, BVariant& _parentValue)
{
	AutoLocker<Team> teamLocker(fThread->GetTeam());

	// If there's already a value for the parent path, we're done.
	StackFrameValues* values = fStackFrame->Values();
	StackFrameValueInfos* valueInfos = fStackFrame->ValueInfos();
	if (values->HasValue(fVariable->ID(), parentPath)) {
		if (!values->GetValue(fVariable->ID(), parentPath, _parentValue)
			|| !valueInfos->GetInfo(fVariable->ID(), parentPath, &_parentType,
					&_parentLocation)) {
			return B_ERROR;
		}

		return B_OK;
	}

	// check whether a job is already in progress
	AutoLocker<Worker> workerLocker(GetWorker());
	GetStackFrameValueJobKey jobKey(fStackFrame, fVariable, parentPath);
	if (GetWorker()->GetJob(jobKey) == NULL) {
		workerLocker.Unlock();

		// schedule the job
		status_t error = GetWorker()->ScheduleJob(
			new(std::nothrow) GetStackFrameValueJob(fDebuggerInterface,
				fArchitecture, fThread, fStackFrame, fVariable, parentPath));
		if (error != B_OK) {
			// scheduling failed -- set the value to invalid
			values->SetValue(fVariable->ID(), parentPath, BVariant());
			return error;
		}
	}

	// wait for the job to finish
	workerLocker.Unlock();
	teamLocker.Unlock();

	switch (WaitFor(jobKey)) {
		case JOB_DEPENDENCY_SUCCEEDED:
		case JOB_DEPENDENCY_NOT_FOUND:
			// "Not found" can happen due to a race condition between
			// unlocking the worker and starting to wait.
			break;
		case JOB_DEPENDENCY_FAILED:
		case JOB_DEPENDENCY_ABORTED:
		default:
			return B_ERROR;
	}

	teamLocker.Lock();

	// now there should be a value for the path
	if (!values->GetValue(fVariable->ID(), parentPath, _parentValue)
		|| !valueInfos->GetInfo(fVariable->ID(), parentPath, &_parentType,
				&_parentLocation)) {
		return B_ERROR;
	}

	return B_OK;
}
