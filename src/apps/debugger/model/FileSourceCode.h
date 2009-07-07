/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_SOURCE_CODE_H
#define FILE_SOURCE_CODE_H

#include <ObjectList.h>

#include "SourceCode.h"


class ContiguousStatement;
class SourceFile;


class FileSourceCode : public SourceCode {
public:
								FileSourceCode(SourceFile* file);
	virtual						~FileSourceCode();

			status_t			Init();
			status_t			AddStatement(ContiguousStatement* statement);

	virtual	int32				CountLines() const;
	virtual	const char*			LineAt(int32 index) const;

	virtual	Statement*			StatementAtLine(int32 index) const;
//			Statement*			StatementAtAddress(target_addr_t address) const;

//	virtual	TargetAddressRange	StatementAddressRange() const;

private:
			typedef BObjectList<ContiguousStatement> StatementList;

private:
	static	int					_CompareStatements(
									const ContiguousStatement* a,
									const ContiguousStatement* b);
	static	int					_CompareAddressStatement(
									const target_addr_t* address,
									const ContiguousStatement* statement);

private:
			SourceFile*			fFile;
			Statement**			fLineStatements;
			StatementList		fStatements;
};


#endif	// FILE_BASED_SOURCE_CODE_H
