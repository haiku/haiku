/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ExpressionValues.h"

#include <new>

#include "FunctionID.h"
#include "StringUtils.h"
#include "Thread.h"


struct ExpressionValues::Key {
	FunctionID*			function;
	Thread*				thread;
	BString				expression;

	Key(FunctionID* function, Thread* thread, const BString& expression)
		:
		function(function),
		thread(thread),
		expression(expression)
	{
	}

	uint32 HashValue() const
	{
		return function->HashValue() ^ thread->ID()
			^ StringUtils::HashValue(expression);
	}

	bool operator==(const Key& other) const
	{
		return *function == *other.function
			&& thread->ID() == other.thread->ID()
			&& expression == other.expression;
	}
};


struct ExpressionValues::ValueEntry : Key {
	BVariant			value;
	ValueEntry*			next;

	ValueEntry(FunctionID* function, Thread* thread, const BString& expression)
		:
		Key(function, thread, expression)
	{
		function->AcquireReference();
		thread->AcquireReference();
	}

	~ValueEntry()
	{
		function->ReleaseReference();
		thread->ReleaseReference();
	}
};


struct ExpressionValues::ValueEntryHashDefinition {
	typedef Key			KeyType;
	typedef	ValueEntry	ValueType;

	size_t HashKey(const Key& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const ValueEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const Key& key, const ValueEntry* value) const
	{
		return key == *value;
	}

	ValueEntry*& GetLink(ValueEntry* value) const
	{
		return value->next;
	}
};


ExpressionValues::ExpressionValues()
	:
	fValues(NULL)
{
}


ExpressionValues::ExpressionValues(const ExpressionValues& other)
	:
	fValues(NULL)
{
	try {
		// init
		if (Init() != B_OK)
			throw std::bad_alloc();

		// clone all values
		for (ValueTable::Iterator it = other.fValues->GetIterator();
				ValueEntry* entry = it.Next();) {
			if (SetValue(entry->function, entry->thread, entry->expression,
				entry->value) != B_OK) {
				throw std::bad_alloc();
			}
		}
	} catch (...) {
		_Cleanup();
		throw;
	}
}


ExpressionValues::~ExpressionValues()
{
	_Cleanup();
}


status_t
ExpressionValues::Init()
{
	fValues = new(std::nothrow) ValueTable;
	if (fValues == NULL)
		return B_NO_MEMORY;

	return fValues->Init();
}


bool
ExpressionValues::GetValue(FunctionID* function, Thread* thread,
	const BString* expression, BVariant& _value) const
{
	ValueEntry* entry = fValues->Lookup(Key(function, thread, *expression));
	if (entry == NULL)
		return false;

	_value = entry->value;
	return true;
}


bool
ExpressionValues::HasValue(FunctionID* function, Thread* thread,
	const BString* expression) const
{
	return fValues->Lookup(Key(function, thread, *expression)) != NULL;
}


status_t
ExpressionValues::SetValue(FunctionID* function, Thread* thread,
	const BString& expression, const BVariant& value)
{
	ValueEntry* entry = fValues->Lookup(Key(function, thread, expression));
	if (entry == NULL) {
		entry = new(std::nothrow) ValueEntry(function, thread, expression);
		if (entry == NULL)
			return B_NO_MEMORY;
		fValues->Insert(entry);
	}

	entry->value = value;
	return B_OK;
}


void
ExpressionValues::_Cleanup()
{
	if (fValues != NULL) {
		ValueEntry* entry = fValues->Clear(true);

		while (entry != NULL) {
			ValueEntry* next = entry->next;
			delete entry;
			entry = next;
		}

		delete fValues;
		fValues = NULL;
	}
}
