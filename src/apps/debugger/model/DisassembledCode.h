/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DISASSEMBLED_CODE_H
#define DISASSEMBLED_CODE_H

#include <ObjectList.h>

#include "SourceCode.h"


class BString;
class ContiguousStatement;


class DisassembledCode : public SourceCode {
public:
								DisassembledCode();
								~DisassembledCode();

	virtual	int32				CountLines() const;
	virtual	const char*			LineAt(int32 index) const;

	virtual	Statement*			StatementAtLine(int32 index) const;
//			Statement*			StatementAtAddress(target_addr_t address) const;

//			TargetAddressRange	StatementAddressRange() const;

public:
			bool				AddCommentLine(const BString& line);
			bool				AddInstructionLine(const BString& line,
									target_addr_t address, target_size_t size);
										// instructions must be added in
										// ascending address order

private:
			struct Line;

			typedef BObjectList<Line> LineList;
			typedef BObjectList<ContiguousStatement> StatementList;

private:
			bool				_AddLine(const BString& line,
									ContiguousStatement* statement);
	static	int					_CompareAddressStatement(
									const target_addr_t* address,
									const ContiguousStatement* statement);

private:
			LineList			fLines;
			StatementList		fStatements;
};


#endif	// DISASSEMBLED_CODE_H
