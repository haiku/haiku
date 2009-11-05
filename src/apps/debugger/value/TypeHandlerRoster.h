/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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

			status_t			FindTypeHandler(ValueNodeChild* nodeChild,
									Type* type, TypeHandler*& _handler);
									// returns a reference
			status_t			CreateValueNode(ValueNodeChild* nodeChild,
									Type* type, ValueNode*& _node);
									// returns a reference

			bool				RegisterHandler(TypeHandler* handler);
			void				UnregisterHandler(TypeHandler* handler);

private:
			BLocker				fLock;
			TypeHandlerList		fTypeHandlers;
	static	TypeHandlerRoster*	sDefaultInstance;
};


#endif	// TYPE_HANDLER_ROSTER_H
