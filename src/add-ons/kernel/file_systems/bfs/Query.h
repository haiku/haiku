#ifndef QUERY_H
#define QUERY_H
/* Query - query parsing and evaluation
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

#include "Index.h"
#include "Stack.h"
#include "Chain.h"

class Volume;
class Term;
class Equation;
class TreeIterator;
class Query;


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
		char *fPosition;
		Term *fTerm;
};

class Query {
	public:
		Query(Volume *volume,Expression *expression);
		~Query();

		status_t GetNextEntry(struct dirent *,size_t size);

		void SetLiveMode(port_id port,int32 token);
		void LiveUpdate(Inode *inode,const char *attribute,int32 type,const uint8 *oldKey,size_t oldLength,const uint8 *newKey,size_t newLength);

		Expression *GetExpression() const { return fExpression; }

	private:
		Volume			*fVolume;
		Expression		*fExpression;
		Equation		*fCurrent;
		TreeIterator	*fIterator;
		Index			fIndex;
		Stack<Equation *> fStack;

		port_id			fPort;
		int32			fToken;

	private:
		friend Chain<Query>;
		Query			*fNext;
};

#endif	/* QUERY_H */
