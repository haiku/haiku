/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_CONDITION_VARIABLE_H
#define _KERNEL_CONDITION_VARIABLE_H


#include <OS.h>

#ifdef __cplusplus

#include <util/OpenHashTable.h>


class PrivateConditionVariable;

class PrivateConditionVariableEntry {
protected:
			bool				Add(const void* object);
			void				Wait();
			void				Wait(const void* object);

protected:
			PrivateConditionVariableEntry* fNext;
			PrivateConditionVariable* fVariable;
			struct thread*		fThread;

			friend class PrivateConditionVariable;
};


struct PrivateConditionVariable
	: protected HashTableLink<PrivateConditionVariable> {
public:
	static	void				ListAll();
			void				Dump();

protected:
			void				Publish(const void* object,
									const char* objectType);
			void				Unpublish();
			void				Notify();

private:
			void				_Notify();

protected:
			const void*			fObject;
			const char*			fObjectType;
			PrivateConditionVariableEntry* fEntries;

			friend class PrivateConditionVariableEntry;
			friend class ConditionVariableHashDefinition;
};


template<typename Type>
class ConditionVariable : private PrivateConditionVariable {
public:
	inline	void				Publish(const Type* object,
									const char* objectType);

	inline	void				Unpublish();
	inline	void				Notify();
};


template<typename Type>
class ConditionVariableEntry : private PrivateConditionVariableEntry {
public:
	inline	bool				Add(const Type* object);
	inline	void				Wait();
	inline	void				Wait(const Type* object);

private:
			bool				fAdded;
};


template<typename Type>
inline void
ConditionVariable<Type>::Publish(const Type* object, const char* objectType)
{
	PrivateConditionVariable::Publish(object, objectType);
}


template<typename Type>
inline void
ConditionVariable<Type>::Unpublish()
{
	PrivateConditionVariable::Unpublish();
}


template<typename Type>
inline void
ConditionVariable<Type>::Notify()
{
	PrivateConditionVariable::Notify();
}


template<typename Type>
inline bool
ConditionVariableEntry<Type>::Add(const Type* object)
{
	return PrivateConditionVariableEntry::Add(object);
}


template<typename Type>
inline void
ConditionVariableEntry<Type>::Wait()
{
	PrivateConditionVariableEntry::Wait();
}


template<typename Type>
inline void
ConditionVariableEntry<Type>::Wait(const Type* object)
{
	PrivateConditionVariableEntry::Wait(object);
}


#if 0
extern void publish_stack_condition_variable(condition_variable* variable,
	const void* object, const char* objectType);
extern void unpublish_condition_variable(const void* object);
extern void notify_condition_variable(const void* object);

extern void wait_for_condition_variable(const void* object);
extern bool add_condition_variable_entry(const void* object,
	condition_variable_entry* entry);
extern void wait_for_condition_variable_entry(
	condition_variable_entry* entry);
#endif	// 0

extern "C" {
#endif	// __cplusplus

extern void condition_variable_init();

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_CONDITION_VARIABLE_H */
