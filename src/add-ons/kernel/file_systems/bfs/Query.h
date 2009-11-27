/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef QUERY_H
#define QUERY_H

#include "system_dependencies.h"

#include "Index.h"


class Volume;
class Term;
class Equation;
class TreeIterator;
class Query;


class Expression {
public:
							Expression(char* expr);
							~Expression();

			status_t		InitCheck();
			const char*		Position() const { return fPosition; }
			Term*			Root() const { return fTerm; }

protected:
			Term*			ParseOr(char** expr);
			Term*			ParseAnd(char** expr);
			Term*			ParseEquation(char** expr);

			bool			IsOperator(char** expr, char op);

private:
							Expression(const Expression& other);
							Expression& operator=(const Expression& other);
								// no implementation

			char*			fPosition;
			Term*			fTerm;
};

class Query : public SinglyLinkedListLinkImpl<Query> {
public:
							Query(Volume* volume, Expression* expression,
								uint32 flags);
							~Query();

			status_t		Rewind();
			status_t		GetNextEntry(struct dirent* , size_t size);

			void			SetLiveMode(port_id port, int32 token);
			void			LiveUpdate(Inode* inode, const char* attribute,
								int32 type, const uint8* oldKey,
								size_t oldLength, const uint8* newKey,
								size_t newLength);
			void			LiveUpdateRenameMove(Inode* inode,
								ino_t oldDirectoryID, const char* oldName,
								size_t oldLength, ino_t newDirectoryID,
								const char* newName, size_t newLength);

			Expression*		GetExpression() const { return fExpression; }

private:
			Volume*			fVolume;
			Expression*		fExpression;
			Equation*		fCurrent;
			TreeIterator*	fIterator;
			Index			fIndex;
			Stack<Equation*> fStack;

			uint32			fFlags;
			port_id			fPort;
			int32			fToken;
};

#endif	// QUERY_H
