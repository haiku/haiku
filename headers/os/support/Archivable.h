/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCHIVABLE_H
#define _ARCHIVABLE_H


#include <BeBuild.h>
#include <image.h>
#include <SupportDefs.h>


class BMessage;


class BArchivable {
	public:
		BArchivable(BMessage* from);
		BArchivable();
		virtual ~BArchivable();	

		virtual	status_t Archive(BMessage* into, bool deep = true) const;
		static BArchivable* Instantiate(BMessage* archive);

		// Private or reserved
		virtual status_t Perform(perform_code d, void* arg);

	private:
		virtual	void _ReservedArchivable1();
		virtual	void _ReservedArchivable2();
		virtual	void _ReservedArchivable3();

		uint32 _reserved[2];
};


// global functions

typedef BArchivable* (*instantiation_func)(BMessage*);

BArchivable* instantiate_object(BMessage *from, image_id *id);
BArchivable* instantiate_object(BMessage *from);
bool validate_instantiation(BMessage* from, const char* className);

instantiation_func find_instantiation_func(const char* className,
	const char* signature);
instantiation_func find_instantiation_func(const char* className);
instantiation_func find_instantiation_func(BMessage* archive);

#endif	// _ARCHIVABLE_H
