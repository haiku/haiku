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


struct PrivateConditionVariableEntry {
public:
	inline	PrivateConditionVariable* Variable() const
		{ return fVariable; }

	inline	PrivateConditionVariableEntry* ThreadNext() const
		{ return fThreadNext; }

	class Private;

protected:
			bool				Add(const void* object,
									PrivateConditionVariableEntry* threadNext);
			void				Wait(uint32 flags);
			void				Wait(const void* object, uint32 flags);

private:
			void				_Remove();

protected:
			PrivateConditionVariableEntry* fVariableNext;
			PrivateConditionVariable* fVariable;
			struct thread*		fThread;
			uint32				fFlags;

			PrivateConditionVariableEntry* fThreadPrevious;
			PrivateConditionVariableEntry* fThreadNext;

			friend class PrivateConditionVariable;
			friend class Private;
};


class PrivateConditionVariable
	: protected HashTableLink<PrivateConditionVariable> {
public:
	static	void				ListAll();
			void				Dump();

protected:
			void				Publish(const void* object,
									const char* objectType);
			void				Unpublish();
			void				Notify(bool all);

private:
			void				_Notify(bool all);

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
	inline	void				NotifyOne();
	inline	void				NotifyAll();
};


template<typename Type>
class ConditionVariableEntry : public PrivateConditionVariableEntry {
public:
	inline	bool				Add(const Type* object,
									PrivateConditionVariableEntry* threadNext
										= NULL);
	inline	void				Wait(uint32 flags = 0);
	inline	void				Wait(const Type* object, uint32 flags = 0);

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
ConditionVariable<Type>::NotifyOne()
{
	PrivateConditionVariable::Notify(false);
}


template<typename Type>
inline void
ConditionVariable<Type>::NotifyAll()
{
	PrivateConditionVariable::Notify(true);
}


template<typename Type>
inline bool
ConditionVariableEntry<Type>::Add(const Type* object,
	PrivateConditionVariableEntry* threadNext)
{
	return PrivateConditionVariableEntry::Add(object, threadNext);
}


template<typename Type>
inline void
ConditionVariableEntry<Type>::Wait(uint32 flags)
{
	PrivateConditionVariableEntry::Wait(flags);
}


template<typename Type>
inline void
ConditionVariableEntry<Type>::Wait(const Type* object, uint32 flags)
{
	PrivateConditionVariableEntry::Wait(object, flags);
}


extern "C" {
#endif	// __cplusplus

struct thread;

extern status_t condition_variable_interrupt_thread(struct thread* thread);

extern void condition_variable_init();

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_CONDITION_VARIABLE_H */
