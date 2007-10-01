/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_CONDITION_VARIABLE_H
#define _KERNEL_CONDITION_VARIABLE_H


#include <OS.h>

#include <debug.h>

#ifdef __cplusplus

#include <util/OpenHashTable.h>


class PrivateConditionVariable;


struct PrivateConditionVariableEntry {
public:
#if KDEBUG
	inline PrivateConditionVariableEntry()
		: fVariable(NULL)
	{
	}

	inline ~PrivateConditionVariableEntry()
	{
		if (fVariable != NULL) {
			panic("Destroying condition variable entry %p, but it's still "
				"attached to variable %p\n", this, fVariable);
		}
	}
#endif

	inline	PrivateConditionVariable* Variable() const
		{ return fVariable; }

	inline	PrivateConditionVariableEntry* ThreadNext() const
		{ return fThreadNext; }
	inline	PrivateConditionVariableEntry* ThreadPrevious() const
		{ return fThreadPrevious; }

	class Private;

protected:
			bool				Add(const void* object,
									PrivateConditionVariableEntry* threadNext);
			status_t			Wait(uint32 flags);
			status_t			Wait(const void* object, uint32 flags);

private:
			void				_Remove();

protected:
			PrivateConditionVariableEntry* fVariableNext;
			PrivateConditionVariable* fVariable;
			struct thread*		fThread;
			uint32				fFlags;
			status_t			fResult;

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
			void				Unpublish(bool threadsLocked);
			void				Notify(bool all, bool threadsLocked);

private:
			void				_Notify(bool all, status_t result);

protected:
			const void*			fObject;
			const char*			fObjectType;
			PrivateConditionVariableEntry* fEntries;

			friend class PrivateConditionVariableEntry;
			friend class ConditionVariableHashDefinition;
};


template<typename Type = void>
class ConditionVariable : private PrivateConditionVariable {
public:
	inline	void				Publish(const Type* object,
									const char* objectType);

	inline	void				Unpublish(bool threadsLocked = false);
	inline	void				NotifyOne(bool threadsLocked = false);
	inline	void				NotifyAll(bool threadsLocked = false);
};


template<typename Type = void>
class ConditionVariableEntry : public PrivateConditionVariableEntry {
public:
	inline	bool				Add(const Type* object,
									PrivateConditionVariableEntry* threadNext
										= NULL);
	inline	status_t			Wait(uint32 flags = 0);
	inline	status_t			Wait(const Type* object, uint32 flags = 0);
};


template<typename Type>
inline void
ConditionVariable<Type>::Publish(const Type* object, const char* objectType)
{
	PrivateConditionVariable::Publish(object, objectType);
}


template<typename Type>
inline void
ConditionVariable<Type>::Unpublish(bool threadsLocked)
{
	PrivateConditionVariable::Unpublish(threadsLocked);
}


template<typename Type>
inline void
ConditionVariable<Type>::NotifyOne(bool threadsLocked)
{
	PrivateConditionVariable::Notify(false, threadsLocked);
}


template<typename Type>
inline void
ConditionVariable<Type>::NotifyAll(bool threadsLocked)
{
	PrivateConditionVariable::Notify(true, threadsLocked);
}


template<typename Type>
inline bool
ConditionVariableEntry<Type>::Add(const Type* object,
	PrivateConditionVariableEntry* threadNext)
{
	return PrivateConditionVariableEntry::Add(object, threadNext);
}


template<typename Type>
inline status_t
ConditionVariableEntry<Type>::Wait(uint32 flags)
{
	return PrivateConditionVariableEntry::Wait(flags);
}


template<typename Type>
inline status_t
ConditionVariableEntry<Type>::Wait(const Type* object, uint32 flags)
{
	return PrivateConditionVariableEntry::Wait(object, flags);
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
