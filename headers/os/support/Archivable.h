/*
 * Copyright 2001-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCHIVABLE_H
#define _ARCHIVABLE_H


#include <image.h>
#include <SupportDefs.h>


class BMessage;

namespace BPrivate {
namespace Archiving {
	class BArchiveManager;
	class BUnarchiveManager;
}
}

using BPrivate::Archiving::BArchiveManager;
using BPrivate::Archiving::BUnarchiveManager;


class BArchivable {
public:
							BArchivable(BMessage* from);
							BArchivable();
	virtual 				~BArchivable();	

	virtual	status_t 		Archive(BMessage* into, bool deep = true) const;
	static 	BArchivable*	Instantiate(BMessage* archive);

	virtual status_t		Perform(perform_code d, void* arg);

	virtual	status_t 		AllUnarchived(const BMessage* archive);
	virtual	status_t 		AllArchived(BMessage* archive) const;

private:
	virtual	void _ReservedArchivable3();

	uint32 _reserved[2];
};


class BArchiver {
public:
							BArchiver(BMessage* archive);
							~BArchiver();

		status_t			AddArchivable(const char* name,
								BArchivable* archivable, bool deep = true);

		status_t			GetTokenForArchivable(BArchivable* archivable,
								int32& _token, bool deep = true);

		bool				IsArchived(BArchivable* archivable);
		status_t			Finish();
		BMessage*			ArchiveMessage() const;

private:
	friend class BArchivable;

							BArchiver(); // not defined
							BArchiver(const BArchiver&); // not defined

		void				RegisterArchivable(const BArchivable* archivable);

		BArchiveManager*	fManager;
		BMessage*			fArchive;
		bool				fFinished;
};


class BUnarchiver {
public:
								BUnarchiver(const BMessage* archive);
								~BUnarchiver();

			status_t			GetArchivable(int32 token,
									BArchivable** archivable);

			status_t			FindArchivable(const char* name,
									BArchivable** archivable);

			status_t			FindArchivable(const char* name, int32 index,
									BArchivable** archivable);

			status_t			EnsureUnarchived(const char* name,
									int32 index = 0);
			status_t			EnsureUnarchived(int32 token);

			bool				IsInstantiated(int32 token);
			bool				IsInstantiated(const char* name,
									int32 index = 0);

			status_t			Finish();
			const BMessage*		ArchiveMessage() const;

	static	bool				IsArchiveManaged(BMessage* archive);
	static	BMessage*			PrepareArchive(BMessage*& archive);
private:
	friend class BArchivable;

								BUnarchiver(); // not defined
								BUnarchiver(const BUnarchiver&); // not defined

			void				RegisterArchivable(BArchivable* archivable);

			void				_CallDebuggerIfManagerNull();

			BUnarchiveManager*	fManager;
			const BMessage*		fArchive;
			bool				fFinished;
};


// global functions

typedef BArchivable* (*instantiation_func)(BMessage*);

BArchivable* instantiate_object(BMessage* from, image_id* id);
BArchivable* instantiate_object(BMessage* from);
bool validate_instantiation(BMessage* from, const char* className);

instantiation_func find_instantiation_func(const char* className,
	const char* signature);
instantiation_func find_instantiation_func(const char* className);
instantiation_func find_instantiation_func(BMessage* archive);

#endif	// _ARCHIVABLE_H
