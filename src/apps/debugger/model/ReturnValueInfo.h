/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef RETURN_VALUE_INFO_H
#define RETURN_VALUE_INFO_H


#include "ObjectList.h"
#include "Referenceable.h"
#include "Types.h"


class CpuState;


class ReturnValueInfo : public BReferenceable {
public:
								ReturnValueInfo();
								ReturnValueInfo(target_addr_t address,
									CpuState* state);
								~ReturnValueInfo();

			void				SetTo(target_addr_t address, CpuState* state);

			target_addr_t		SubroutineAddress() const
									{ return fAddress; }
			CpuState* 			State() const	{ return fState; }

private:
			target_addr_t		fAddress;
			CpuState*			fState;
};


typedef BObjectList<ReturnValueInfo> ReturnValueInfoList;


#endif	// RETURN_VALUE_INFO_H
