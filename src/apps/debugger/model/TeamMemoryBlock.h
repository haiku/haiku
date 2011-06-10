/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_MEMORY_BLOCK_H
#define TEAM_MEMORY_BLOCK_H


#include <OS.h>

#include <Locker.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "Types.h"


class TeamMemoryBlockOwner;


class TeamMemoryBlock : public BReferenceable,
	public DoublyLinkedListLinkImpl<TeamMemoryBlock> {
public:
	class Listener;

public:
							TeamMemoryBlock(target_addr_t baseAddress,
								TeamMemoryBlockOwner* owner);
							~TeamMemoryBlock();

			void			AddListener(Listener* listener);
			bool			HasListener(Listener* listener);
			void			RemoveListener(Listener* listener);

			bool			IsValid() const 		{ return fValid; };
			void			MarkValid();
			void			Invalidate();

			target_addr_t	BaseAddress() const 	{ return fBaseAddress; };
			uint8*			Data() 					{ return fData; };
			size_t			Size() const			{ return sizeof(fData); };

			void			NotifyDataRetrieved();

protected:
	virtual void			LastReferenceReleased();

private:
			typedef			DoublyLinkedList<Listener> ListenerList;

private:
			bool			fValid;
			target_addr_t	fBaseAddress;
			uint8			fData[B_PAGE_SIZE];
			ListenerList	fListeners;
			TeamMemoryBlockOwner*
							fBlockOwner;
			BLocker			fLock;
};


class TeamMemoryBlock::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual					~Listener();

	virtual void			MemoryBlockRetrieved(TeamMemoryBlock* block);
};


#endif // TEAM_MEMORY_BLOCK_H

