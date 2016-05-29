/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_VALUES_H
#define STACK_FRAME_VALUES_H


#include <Referenceable.h>
#include <util/OpenHashTable.h>
#include <Variant.h>


class ObjectID;
class TypeComponentPath;


class StackFrameValues : public BReferenceable {
public:
								StackFrameValues();
								StackFrameValues(const StackFrameValues& other);
									// throws std::bad_alloc
	virtual						~StackFrameValues();

			status_t			Init();

			bool				GetValue(ObjectID* variable,
									const TypeComponentPath* path,
									BVariant& _value) const;
	inline	bool				GetValue(ObjectID* variable,
									const TypeComponentPath& path,
									BVariant& _value) const;
			bool				HasValue(ObjectID* variable,
									const TypeComponentPath* path) const;
	inline	bool				HasValue(ObjectID* variable,
									const TypeComponentPath& path) const;
			status_t			SetValue(ObjectID* variable,
									TypeComponentPath* path,
									const BVariant& value);

private:
			struct Key;
			struct ValueEntry;
			struct ValueEntryHashDefinition;

			typedef BOpenHashTable<ValueEntryHashDefinition> ValueTable;

private:
			StackFrameValues&	operator=(const StackFrameValues& other);

			void				_Cleanup();

private:
			ValueTable*			fValues;
};


bool
StackFrameValues::GetValue(ObjectID* variable, const TypeComponentPath& path,
	BVariant& _value) const
{
	return GetValue(variable, &path, _value);
}


bool
StackFrameValues::HasValue(ObjectID* variable, const TypeComponentPath& path)
	const
{
	return HasValue(variable, &path);
}


#endif	// STACK_FRAME_VALUES_H
