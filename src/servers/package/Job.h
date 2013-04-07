/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef JOB_H
#define JOB_H


#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Job : public BReferenceable, public DoublyLinkedListLinkImpl<Job> {
public:
								Job();
	virtual						~Job();

	virtual	void				Do() = 0;
};


#endif	// JOB_H
