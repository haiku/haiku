/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef FULL_TEXT_ANALYSER_H
#define FULL_TEXT_ANALYSER_H


#include "IndexServerAddOn.h"

#include <Path.h>

#include "TextDataBase.h"


const char* kFullTextDirectory = "FullTextAnalyser";


class FullTextAnalyser : public FileAnalyser {
public:
								FullTextAnalyser(BString name,
									const BVolume& volume);
								~FullTextAnalyser();

			status_t			InitCheck();

			void				AnalyseEntry(const entry_ref& ref);
			void				DeleteEntry(const entry_ref& ref);
			void				MoveEntry(const entry_ref& oldRef,
									const entry_ref& newRef);
			void				LastEntry();

private:
	inline	bool				_InterestingEntry(const entry_ref& ref);
	inline	bool				_IsInIndexDirectory(const entry_ref& ref);

			TextWriteDataBase*	fWriteDataBase;
			BPath				fDataBasePath;

			uint32				fNUncommited;
};


class FullTextAddOn : public IndexServerAddOn {
public:
								FullTextAddOn(image_id id, const char* name);

			FileAnalyser*		CreateFileAnalyser(const BVolume& volume);
};

#endif
