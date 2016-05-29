/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_VALUE_INFOS_H
#define STACK_FRAME_VALUE_INFOS_H


#include <Referenceable.h>
#include <util/OpenHashTable.h>
#include <Variant.h>


class ObjectID;
class Type;
class TypeComponentPath;
class ValueLocation;


class StackFrameValueInfos : public BReferenceable {
public:
								StackFrameValueInfos();
	virtual						~StackFrameValueInfos();

			status_t			Init();

			bool				GetInfo(ObjectID* variable,
									const TypeComponentPath* path,
									Type** _type, ValueLocation** _location)
										const;
									// returns references
	inline	bool				GetInfo(ObjectID* variable,
									const TypeComponentPath& path,
									Type** _type, ValueLocation** _location)
										const;
									// returns references
			bool				HasInfo(ObjectID* variable,
									const TypeComponentPath* path) const;
	inline	bool				HasInfo(ObjectID* variable,
									const TypeComponentPath& path) const;
			status_t			SetInfo(ObjectID* variable,
									TypeComponentPath* path, Type* type,
									ValueLocation* location);

private:
			struct Key;
			struct InfoEntry;
			struct InfoEntryHashDefinition;

			typedef BOpenHashTable<InfoEntryHashDefinition> ValueTable;

private:
			StackFrameValueInfos&	operator=(const StackFrameValueInfos& other);

			void				_Cleanup();

private:
			ValueTable*			fValues;
};


bool
StackFrameValueInfos::GetInfo(ObjectID* variable, const TypeComponentPath& path,
	Type** _type, ValueLocation** _location) const
{
	return GetInfo(variable, &path, _type, _location);
}


bool
StackFrameValueInfos::HasInfo(ObjectID* variable, const TypeComponentPath& path)
	const
{
	return HasInfo(variable, &path);
}


#endif	// STACK_FRAME_VALUE_INFOS_H
