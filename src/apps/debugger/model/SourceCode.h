/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_CODE_H
#define SOURCE_CODE_H

#include <ObjectList.h>
#include <Referenceable.h>

#include "ArchitectureTypes.h"


class Statement;


class SourceCode : public Referenceable {
public:
								SourceCode();
								~SourceCode();

			int32				CountLines() const;
			const char*			LineAt(int32 index) const;

			int32				CountStatements() const;
			Statement*			StatementAt(int32 index) const;

			bool				AddLine(const char* line);
									// clones
			bool				AddStatement(Statement* statement);
									// takes over reference

private:
			typedef BObjectList<char> LineList;
			typedef BObjectList<Statement> StatementList;

private:
			LineList			fLines;
			StatementList		fStatements;
};


#endif	// SOURCE_CODE_H
