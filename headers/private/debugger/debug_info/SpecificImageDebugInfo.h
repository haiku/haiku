/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPECIFIC_IMAGE_DEBUG_INFO_H
#define SPECIFIC_IMAGE_DEBUG_INFO_H


#include <ObjectList.h>
#include <Referenceable.h>

#include "AddressSectionTypes.h"
#include "ReturnValueInfo.h"
#include "Types.h"


class Architecture;
class BString;
class CpuState;
class DataMember;
class DebuggerInterface;
class FileSourceCode;
class FunctionDebugInfo;
class FunctionInstance;
class GlobalTypeCache;
class Image;
class ImageInfo;
class LocatableFile;
class SourceLanguage;
class SourceLocation;
class StackFrame;
class Statement;
class SymbolInfo;
class Type;
class TypeLookupConstraints;
class ValueLocation;


class SpecificImageDebugInfo : public BReferenceable {
public:
	virtual						~SpecificImageDebugInfo();

	virtual	status_t			GetFunctions(
									const BObjectList<SymbolInfo, true>& symbols,
									BObjectList<FunctionDebugInfo>& functions)
										= 0;
									// returns references

	virtual	status_t			GetType(GlobalTypeCache* cache,
									const BString& name,
									const TypeLookupConstraints& constraints,
									Type*& _type) = 0;
									// returns a reference
	virtual	bool				HasType(const BString& name,
									const TypeLookupConstraints& constraints)
									const = 0;

	virtual AddressSectionType	GetAddressSectionType(target_addr_t address)
									= 0;

	virtual	status_t			CreateFrame(Image* image,
									FunctionInstance* functionInstance,
									CpuState* cpuState,
									bool getFullFrameInfo,
									ReturnValueInfoList* returnValueInfos,
									StackFrame*& _Frame,
									CpuState*& _previousCpuState) = 0;
										// returns reference to previous frame
										// and CPU state; returned CPU state
										// can be NULL; can return B_UNSUPPORTED
										// getFullFrameInfo: try to retrieve
										// variables/parameters if true
										// (and supported)
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement) = 0;
										// returns reference
	virtual	status_t			GetStatementAtSourceLocation(
									FunctionDebugInfo* function,
									const SourceLocation& sourceLocation,
									Statement*& _statement) = 0;
										// returns reference

	virtual	status_t			GetSourceLanguage(FunctionDebugInfo* function,
									SourceLanguage*& _language) = 0;

	virtual	ssize_t				ReadCode(target_addr_t address, void* buffer,
									size_t size) = 0;

	virtual	status_t			AddSourceCodeInfo(LocatableFile* file,
									FileSourceCode* sourceCode) = 0;

protected:
	static	status_t			GetFunctionsFromSymbols(
									const BObjectList<SymbolInfo, true>& symbols,
									BObjectList<FunctionDebugInfo>& functions,
									DebuggerInterface* interface,
									const ImageInfo& imageInfo,
									SpecificImageDebugInfo* info);

};


#endif	// SPECIFIC_IMAGE_DEBUG_INFO_H
