/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <OS.h>
#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Thread : public Referenceable, public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(thread_id threadID);
								~Thread();

			status_t			Init();

			thread_id			ID() const	{ return fID; }

			const char*			Name() const	{ return fName.String(); }
			void				SetName(const BString& name);


private:
			thread_id			fID;
			BString				fName;
};


typedef DoublyLinkedList<Thread> ThreadList;


#endif	// THREAD_H
