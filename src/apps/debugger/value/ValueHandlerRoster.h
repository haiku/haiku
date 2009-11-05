/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_HANDLER_ROSTER_H
#define VALUE_HANDLER_ROSTER_H


#include <Locker.h>
#include <ObjectList.h>


class TableCellValueRenderer;
class Value;
class ValueFormatter;
class ValueHandler;


typedef BObjectList<ValueHandler> ValueHandlerList;


class ValueHandlerRoster {
public:
								ValueHandlerRoster();
								~ValueHandlerRoster();

	static	ValueHandlerRoster*	Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			status_t			Init();
			status_t			RegisterDefaultHandlers();

			status_t			FindValueHandler(Value* value,
									ValueHandler*& _handler);
									// returns a reference
			status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
									// returns a reference
			status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer);
									// returns a reference

			bool				RegisterHandler(ValueHandler* handler);
			void				UnregisterHandler(ValueHandler* handler);

private:
			BLocker				fLock;
			ValueHandlerList	fValueHandlers;
	static	ValueHandlerRoster*	sDefaultInstance;
};


#endif	// VALUE_HANDLER_ROSTER_H
