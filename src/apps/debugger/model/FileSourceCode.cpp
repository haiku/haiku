/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FileSourceCode.h"

#include <string.h>

#include "SourceFile.h"
#include "Statement.h"


// TODO: Lot's of code duplication from DissassembledCode!


FileSourceCode::FileSourceCode(SourceFile* file)
	:
	fFile(file),
	fLineStatements(NULL)
{
	fFile->AcquireReference();
}


FileSourceCode::~FileSourceCode()
{
	for (int32 i = 0; Statement* statement = fStatements.ItemAt(i); i++)
		statement->RemoveReference();

	delete[] fLineStatements;

	fFile->ReleaseReference();
}


status_t
FileSourceCode::Init()
{
	fLineStatements = new(std::nothrow) Statement*[fFile->CountLines()];
	if (fLineStatements == NULL)
		return B_NO_MEMORY;

	memset(fLineStatements, 0, fFile->CountLines() * sizeof(Statement*));

	return B_OK;
}


status_t
FileSourceCode::AddStatement(ContiguousStatement* statement)
{
	if (!fStatements.BinaryInsert(statement, &_CompareStatements))
		return B_NO_MEMORY;

	int32 line = statement->StartSourceLocation().Line();
	if (line >= 0 && line < fFile->CountLines()
		&& fLineStatements[line] == NULL) {
		fLineStatements[line] = statement;
	}

	statement->AcquireReference();
	return B_OK;
}


int32
FileSourceCode::CountLines() const
{
	return fFile->CountLines();
}


const char*
FileSourceCode::LineAt(int32 index) const
{
	return fFile->LineAt(index);
}


Statement*
FileSourceCode::StatementAtLine(int32 index) const
{
	return index >= 0 && index < CountLines() ? fLineStatements[index] : NULL;
}


//Statement*
//FileSourceCode::StatementAtAddress(target_addr_t address) const
//{
//	return fStatements.BinarySearchByKey(address, &_CompareAddressStatement);
//}


//TargetAddressRange
//FileSourceCode::StatementAddressRange() const
//{
//	if (fStatements.IsEmpty())
//		return TargetAddressRange();
//
//	ContiguousStatement* first = fStatements.ItemAt(0);
//	ContiguousStatement* last
//		= fStatements.ItemAt(fStatements.CountItems() - 1);
//	return TargetAddressRange(first->AddressRange().Start(),
//		last->AddressRange().End());
//}


/*static*/ int
FileSourceCode::_CompareStatements(const ContiguousStatement* a,
	const ContiguousStatement* b)
{
	target_addr_t addressA = a->AddressRange().Start();
	target_addr_t addressB = b->AddressRange().Start();
	if (addressA < addressB)
		return -1;
	return addressA == addressB ? 0 : 1;
}


/*static*/ int
FileSourceCode::_CompareAddressStatement(const target_addr_t* address,
	const ContiguousStatement* statement)
{
	const TargetAddressRange& range = statement->AddressRange();

	if (*address < range.Start())
		return -1;
	return *address < range.End() ? 0 : 1;
}
