/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYMBOL_INFO_H
#define SYMBOL_INFO_H

#include <String.h>

#include "Types.h"


class SymbolInfo {
public:
								SymbolInfo();
								SymbolInfo(target_addr_t address,
									target_size_t size, uint32 type,
									const BString& name);
								~SymbolInfo();

			void				SetTo(target_addr_t address, target_size_t size,
									uint32 type, const BString& name);

			target_addr_t		Address() const		{ return fAddress; }
			target_size_t		Size() const		{ return fSize; }
			uint32				Type() const		{ return fType; }
			const char*			Name() const		{ return fName.String(); }

private:
			target_addr_t		fAddress;
			target_size_t		fSize;
			uint32				fType;
			BString				fName;
};


#endif	// SYMBOL_INFO_H
