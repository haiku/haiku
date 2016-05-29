/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_H
#define FUNCTION_H

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "FunctionInstance.h"
#include "LocatableFile.h"


class FileSourceCode;


class Function : public BReferenceable, private LocatableFile::Listener {
public:
	class Listener;

public:
								Function();
								~Function();

			// team must be locked to access the instances
			FunctionInstance*	FirstInstance() const
									{ return fInstances.Head(); }
			FunctionInstance*	LastInstance() const
									{ return fInstances.Tail(); }
			const FunctionInstanceList& Instances() const
									{ return fInstances; }

			const BString&		Name() const
									{ return FirstInstance()->Name(); }
			const BString&		PrettyName() const
									{ return FirstInstance()->PrettyName(); }
			LocatableFile*		SourceFile() const
									{ return FirstInstance()->SourceFile(); }
			SourceLocation		GetSourceLocation() const
									{ return FirstInstance()
										->GetSourceLocation(); }

			FunctionID*			GetFunctionID() const
									{ return FirstInstance()->GetFunctionID(); }
									// returns a reference

			// mutable attributes follow (locking required)
			FileSourceCode*		GetSourceCode() const	{ return fSourceCode; }
			function_source_state SourceCodeState() const
									{ return fSourceCodeState; }
			void				SetSourceCode(FileSourceCode* source,
									function_source_state state);

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// package private
			void				AddInstance(FunctionInstance* instance);
			void				RemoveInstance(FunctionInstance* instance);

			void				NotifySourceCodeChanged();

			void				LocatableFileChanged(LocatableFile* file);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			FunctionInstanceList fInstances;
			FileSourceCode*		fSourceCode;
			function_source_state fSourceCodeState;
			ListenerList		fListeners;
			int32				fNotificationsDisabled;

public:
			// BOpenHashTable support
			Function*			fNext;
};


class Function::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				FunctionSourceCodeChanged(Function* function);
									// called with lock held
};


#endif	// FUNCTION_H
