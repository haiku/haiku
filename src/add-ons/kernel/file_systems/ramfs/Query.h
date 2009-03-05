/* Query - query parsing and evaluation
 *
 * Copyright 2001-2004, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Adjusted by Ingo Weinhold <bonefish@cs.tu-berlin.de> for usage in RAM FS.
 */
#ifndef QUERY_H
#define QUERY_H


#include <OS.h>
#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include "Index.h"
#include "Stack.h"
#include "ramfs.h"

class Entry;
class Equation;
class IndexIterator;
class Node;
class Query;
class Term;
class Volume;


#define B_QUERY_NON_INDEXED	0x00000002


// Wraps the RAM FS Index to provide the interface required by the Query
// implementation. At least most of it.
//
// IndexWrapper
class IndexWrapper {
public:
	IndexWrapper(Volume *volume);

	status_t SetTo(const char *name);
	void Unset();

	uint32 Type() const;
	off_t GetSize() const;
	int32 KeySize() const;

private:
	friend class IndexIterator;

	Volume	*fVolume;
	Index	*fIndex;
};

// IndexIterator
class IndexIterator {
public:
	IndexIterator(IndexWrapper *indexWrapper);

	status_t Find(const uint8 *const key, size_t keyLength);
	status_t Rewind();
	status_t GetNextEntry(uint8 *buffer, uint16 *keyLength, size_t bufferSize,
						  Entry **entry);

private:
	IndexWrapper		*fIndexWrapper;
	IndexEntryIterator	fIterator;
	bool				fInitialized;
};


class Expression {
	public:
		Expression(char *expr);
		~Expression();

		status_t InitCheck();
		const char *Position() const { return fPosition; }
		Term *Root() const { return fTerm; }

	protected:
		Term *ParseOr(char **expr);
		Term *ParseAnd(char **expr);
		Term *ParseEquation(char **expr);

		bool IsOperator(char **expr,char op);

	private:
		Expression(const Expression &);
		Expression &operator=(const Expression &);
			// no implementation

		char *fPosition;
		Term *fTerm;
};

class Query : public DoublyLinkedListLinkImpl<Query> {
	public:
		Query(Volume *volume, Expression *expression, uint32 flags);
		~Query();

		status_t Rewind();
		status_t GetNextEntry(struct dirent *, size_t size);

		void SetLiveMode(port_id port, int32 token);
		void LiveUpdate(Entry *entry, Node* node, const char *attribute,
			int32 type, const uint8 *oldKey, size_t oldLength,
			const uint8 *newKey, size_t newLength);

		Expression *GetExpression() const { return fExpression; }

	private:
//		void SendNotification(Entry* entry)

	private:
		Volume			*fVolume;
		Expression		*fExpression;
		Equation		*fCurrent;
		IndexIterator	*fIterator;
		IndexWrapper	fIndex;
		Stack<Equation *> fStack;

		uint32			fFlags;
		port_id			fPort;
		int32			fToken;
		bool			fNeedsEntry;
};

#endif	/* QUERY_H */
