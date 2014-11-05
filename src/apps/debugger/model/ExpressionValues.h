/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_VALUES_H
#define EXPRESSION_VALUES_H


#include <String.h>

#include <Referenceable.h>
#include <util/OpenHashTable.h>
#include <Variant.h>


class FunctionID;
class Thread;


class ExpressionValues : public BReferenceable {
public:
								ExpressionValues();
								ExpressionValues(const ExpressionValues& other);
									// throws std::bad_alloc
	virtual						~ExpressionValues();

			status_t			Init();

			bool				GetValue(FunctionID* function,
									Thread* thread,
									const BString* expression,
									BVariant& _value) const;
	inline	bool				GetValue(FunctionID* function,
									Thread* thread,
									const BString& expression,
									BVariant& _value) const;
			bool				HasValue(FunctionID* function,
									Thread* thread,
									const BString* expression) const;
	inline	bool				HasValue(FunctionID* function,
									Thread* thread,
									const BString& expression) const;
			status_t			SetValue(FunctionID* function,
									Thread* thread,
									const BString& expression,
									const BVariant& value);

private:
			struct Key;
			struct ValueEntry;
			struct ValueEntryHashDefinition;

			typedef BOpenHashTable<ValueEntryHashDefinition> ValueTable;

private:
			ExpressionValues&	operator=(const ExpressionValues& other);

			void				_Cleanup();

private:
			ValueTable*			fValues;
};


bool
ExpressionValues::GetValue(FunctionID* function, Thread* thread,
	const BString& expression, BVariant& _value) const
{
	return GetValue(function, thread, &expression, _value);
}


bool
ExpressionValues::HasValue(FunctionID* function, Thread* thread,
	const BString& expression) const
{
	return HasValue(function, thread, &expression);
}


#endif	// EXPRESSION_VALUES_H
