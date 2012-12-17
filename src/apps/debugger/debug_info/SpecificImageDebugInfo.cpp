/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SpecificImageDebugInfo.h"

#include "BasicFunctionDebugInfo.h"
#include "DebuggerInterface.h"
#include "Demangler.h"
#include "ImageInfo.h"
#include "SymbolInfo.h"


SpecificImageDebugInfo::~SpecificImageDebugInfo()
{
}


/*static*/ status_t
SpecificImageDebugInfo::GetFunctionsFromSymbols(
	BObjectList<FunctionDebugInfo>& functions, DebuggerInterface* interface,
	const ImageInfo& imageInfo, SpecificImageDebugInfo* info)
{
	BObjectList<SymbolInfo> symbols(20, true);
	status_t error = interface->GetSymbolInfos(imageInfo.TeamID(),
		imageInfo.ImageID(), symbols);
	if (error != B_OK)
		return error;

	// sort the symbols -- not necessary, but a courtesy to ImageDebugInfo which
	// will peform better when inserting functions at the end of a list
	symbols.SortItems(&_CompareSymbols);

	// create the function infos
	int32 functionsAdded = 0;
	for (int32 i = 0; SymbolInfo* symbol = symbols.ItemAt(i); i++) {
		if (symbol->Type() != B_SYMBOL_TYPE_TEXT)
			continue;

		FunctionDebugInfo* function = new(std::nothrow) BasicFunctionDebugInfo(
			info, symbol->Address(), symbol->Size(), symbol->Name(),
			Demangler::Demangle(symbol->Name()));
		if (function == NULL || !functions.AddItem(function)) {
			delete function;
			int32 index = functions.CountItems() - 1;
			for (; functionsAdded >= 0; functionsAdded--, index--) {
				function = functions.RemoveItemAt(index);
				delete function;
			}
			return B_NO_MEMORY;
		}

		functionsAdded++;
	}

	return B_OK;
}

/*static*/ int
SpecificImageDebugInfo::_CompareSymbols(const SymbolInfo* a,
	const SymbolInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}
