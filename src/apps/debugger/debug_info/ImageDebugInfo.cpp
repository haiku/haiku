/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageDebugInfo.h"

#include <new>

#include "FunctionDebugInfo.h"
#include "FunctionInstance.h"
#include "SpecificImageDebugInfo.h"


ImageDebugInfo::ImageDebugInfo(const ImageInfo& imageInfo)
	:
	fImageInfo(imageInfo)
{
}


ImageDebugInfo::~ImageDebugInfo()
{
	for (int32 i = 0; FunctionInstance* function = fFunctions.ItemAt(i); i++)
		function->ReleaseReference();
}


bool
ImageDebugInfo::AddSpecificInfo(SpecificImageDebugInfo* info)
{
	return fSpecificInfos.AddItem(info);
}


status_t
ImageDebugInfo::FinishInit()
{
	// get functions -- get them from most expressive debug info first and add
	// missing functions from less expressive debug infos
	for (int32 i = 0; SpecificImageDebugInfo* specificInfo
			= fSpecificInfos.ItemAt(i); i++) {
		BObjectList<FunctionDebugInfo> functions;
		status_t error = specificInfo->GetFunctions(functions);
		if (error != B_OK)
			return error;

		for (int32 k = 0; FunctionDebugInfo* function = functions.ItemAt(k);
				k++) {
			if (FunctionAtAddress(function->Address()) != NULL)
				continue;

			FunctionInstance* instance = new(std::nothrow) FunctionInstance(
				this, function);
			if (instance == NULL
				|| !fFunctions.BinaryInsert(instance, &_CompareFunctions)) {
				delete instance;
				error = B_NO_MEMORY;
				break;
			}
		}

		// Remove references returned by the specific debug info -- the
		// FunctionInstance objects have references, now.
		for (int32 k = 0; FunctionDebugInfo* function = functions.ItemAt(k);
				k++) {
			function->RemoveReference();
		}

		if (error != B_OK)
			return error;
	}

	return B_OK;
}


int32
ImageDebugInfo::CountFunctions() const
{
	return fFunctions.CountItems();
}


FunctionInstance*
ImageDebugInfo::FunctionAt(int32 index) const
{
	return fFunctions.ItemAt(index);
}


FunctionInstance*
ImageDebugInfo::FunctionAtAddress(target_addr_t address) const
{
	return fFunctions.BinarySearchByKey(address, &_CompareAddressFunction);
}


status_t
ImageDebugInfo::AddSourceCodeInfo(LocatableFile* file,
	FileSourceCode* sourceCode) const
{
	bool addedAny = false;
	for (int32 i = 0; SpecificImageDebugInfo* specificInfo
			= fSpecificInfos.ItemAt(i); i++) {
		status_t error = specificInfo->AddSourceCodeInfo(file, sourceCode);
		if (error == B_NO_MEMORY)
			return error;
		addedAny |= error == B_OK;
	}

	return addedAny ? B_OK : B_ENTRY_NOT_FOUND;
}


/*static*/ int
ImageDebugInfo::_CompareFunctions(const FunctionInstance* a,
	const FunctionInstance* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}


/*static*/ int
ImageDebugInfo::_CompareAddressFunction(const target_addr_t* address,
	const FunctionInstance* function)
{
	if (*address < function->Address())
		return -1;
	return *address < function->Address() + function->Size() ? 0 : 1;
}
