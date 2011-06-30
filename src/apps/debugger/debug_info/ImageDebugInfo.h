/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_INFO_H
#define IMAGE_DEBUG_INFO_H


#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>

#include "AddressSectionTypes.h"
#include "ImageInfo.h"
#include "Types.h"


class Architecture;
class DebuggerInterface;
class FileSourceCode;
class FunctionDebugInfo;
class FunctionInstance;
class GlobalTypeCache;
class LocatableFile;
class SpecificImageDebugInfo;
class Type;
class TypeLookupConstraints;


class ImageDebugInfo : public BReferenceable {
public:
								ImageDebugInfo(const ImageInfo& imageInfo);
								~ImageDebugInfo();

			const ImageInfo&	GetImageInfo() const	{ return fImageInfo; }

			bool				AddSpecificInfo(SpecificImageDebugInfo* info);
			status_t			FinishInit();

			status_t			GetType(GlobalTypeCache* cache,
									const BString& name,
									const TypeLookupConstraints& constraints,
									Type*& _type);
									// returns a reference
			AddressSectionType	GetAddressSectionType(target_addr_t address)
									const;

			int32				CountFunctions() const;
			FunctionInstance*	FunctionAt(int32 index) const;
			FunctionInstance*	FunctionAtAddress(target_addr_t address) const;
			FunctionInstance*	FunctionByName(const char* name) const;

			status_t			AddSourceCodeInfo(LocatableFile* file,
									FileSourceCode* sourceCode) const;

private:
			typedef BObjectList<SpecificImageDebugInfo> SpecificInfoList;
			typedef BObjectList<FunctionInstance> FunctionList;

private:
	static	int					_CompareFunctions(const FunctionInstance* a,
									const FunctionInstance* b);
	static	int					_CompareAddressFunction(
									const target_addr_t* address,
									const FunctionInstance* function);

private:
			ImageInfo			fImageInfo;
			SpecificInfoList	fSpecificInfos;
			FunctionList		fFunctions;
};


#endif	// IMAGE_DEBUG_INFO_H
