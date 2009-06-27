/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageDebugInfo.h"

#include <new>

#include "FunctionDebugInfo.h"
#include "SpecificImageDebugInfo.h"


ImageDebugInfo::ImageDebugInfo(const ImageInfo& imageInfo)
	:
	fImageInfo(imageInfo)
{
}


ImageDebugInfo::~ImageDebugInfo()
{
	for (int32 i = 0; FunctionDebugInfo* function = fFunctions.ItemAt(i); i++)
		function->RemoveReference();
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
		FunctionList functions;
		status_t error = specificInfo->GetFunctions(functions);
		if (error != B_OK)
			return error;

		for (int32 k = 0; FunctionDebugInfo* function = functions.ItemAt(k);
				k++) {
			if (FunctionAtAddress(function->Address()) == NULL) {
				if (!fFunctions.BinaryInsert(function, &_CompareFunctions)) {
					for (; (function = functions.ItemAt(k)); k++) {
						function->RemoveReference();
						return B_NO_MEMORY;
					}
				}
			} else
				function->RemoveReference();
		}
	}

	return B_OK;
}


int32
ImageDebugInfo::CountFunctions() const
{
	return fFunctions.CountItems();
}


FunctionDebugInfo*
ImageDebugInfo::FunctionAt(int32 index) const
{
	return fFunctions.ItemAt(index);
}


FunctionDebugInfo*
ImageDebugInfo::FunctionAtAddress(target_addr_t address) const
{
	return fFunctions.BinarySearchByKey(address, &_CompareAddressFunction);
}


/*static*/ int
ImageDebugInfo::_CompareFunctions(const FunctionDebugInfo* a,
	const FunctionDebugInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}


/*static*/ int
ImageDebugInfo::_CompareAddressFunction(const target_addr_t* address,
	const FunctionDebugInfo* function)
{
	if (*address < function->Address())
		return -1;
	return *address < function->Address() + function->Size() ? 0 : 1;
}
