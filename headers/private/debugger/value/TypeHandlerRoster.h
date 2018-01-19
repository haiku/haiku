/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_HANDLER_ROSTER_H
#define TYPE_HANDLER_ROSTER_H


#include <Locker.h>
#include <ObjectList.h>


class Type;
class TypeHandler;
class ValueNode;
class ValueNodeChild;


typedef BObjectList<TypeHandler> TypeHandlerList;


class TypeHandlerRoster {
public:
								TypeHandlerRoster();
								~TypeHandlerRoster();

	static	TypeHandlerRoster*	Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			status_t			Init();
			status_t			RegisterDefaultHandlers();

			int32				CountTypeHandlers(Type* type);
			status_t			FindBestTypeHandler(ValueNodeChild* nodeChild,
									Type* type, TypeHandler*& _handler);
									// returns a reference
			status_t			FindTypeHandlers(ValueNodeChild* nodeChild,
									Type* type, TypeHandlerList*& _handlers);
									// returns list of references
			status_t			CreateValueNode(ValueNodeChild* nodeChild,
									Type* type, TypeHandler* handler,
									ValueNode*& _node);
									// handler can be null if automatic
									// search is desired.
									// returns a reference

			bool				RegisterHandler(TypeHandler* handler);
			void				UnregisterHandler(TypeHandler* handler);

private:
			BLocker				fLock;
			TypeHandlerList		fTypeHandlers;
	static	TypeHandlerRoster*	sDefaultInstance;
};


#endif	// TYPE_HANDLER_ROSTER_H
