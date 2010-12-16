/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUB_WINDOW_MANAGER_H
#define SUB_WINDOW_MANAGER_H

#include <Locker.h>

#include <Referenceable.h>

#include <util/OpenHashTable.h>

#include "SubWindow.h"


class SubWindowManager : public BReferenceable, public BLocker {
public:
								SubWindowManager(BLooper* parent);
	virtual						~SubWindowManager();

			status_t			Init();

			bool				AddSubWindow(SubWindow* window);
			bool				RemoveSubWindow(SubWindow* window);
			SubWindow*			LookupSubWindow(const SubWindowKey& key) const;

			void				Broadcast(uint32 messageCode);
			void				Broadcast(BMessage* message);

private:
			struct HashDefinition {
				typedef SubWindowKey	KeyType;
				typedef	SubWindow		ValueType;

				size_t HashKey(const SubWindowKey& key) const
				{
					return key.HashCode();
				}

				size_t Hash(const SubWindow* value) const
				{
					return value->GetSubWindowKey()->HashCode();
				}

				bool Compare(const SubWindowKey& key,
					const SubWindow* value) const
				{
					return key.Equals(value->GetSubWindowKey());
				}

				SubWindow*& GetLink(SubWindow* value) const
				{
					return value->fNext;
				}
			};

			typedef BOpenHashTable<HashDefinition> SubWindowTable;

private:
			BLooper*			fParent;
			SubWindowTable		fSubWindows;
};


#endif	// SUB_WINDOW_MANAGER_H
