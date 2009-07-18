/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_ID_H
#define FUNCTION_ID_H


#include <String.h>

#include "ObjectID.h"


class FunctionID : public ObjectID {
public:
								FunctionID(const BString& idString);
	virtual						~FunctionID();

	virtual	bool				operator==(const ObjectID& other) const;

protected:
	virtual	uint32				ComputeHashValue() const;

private:
			const BString		fIDString;
};


#endif	// FUNCTION_ID_H
