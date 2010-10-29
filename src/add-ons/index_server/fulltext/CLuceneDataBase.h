/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef CLUCENE_DATA_BASE_H
#define CLUCENE_DATA_BASE_H


#include <vector>

#include <Path.h>

#include "TextDataBase.h"

#include <CLucene.h>


using namespace lucene::index;
using namespace lucene::analysis::standard;


class CLuceneWriteDataBase : public TextWriteDataBase {
public:
								CLuceneWriteDataBase(const BPath& databasePath);
								~CLuceneWriteDataBase();

			status_t			InitCheck();

			status_t			AddDocument(const entry_ref& ref);
			status_t			RemoveDocument(const entry_ref& ref);
			status_t			Commit();

private:
			IndexWriter*		_OpenIndexWriter();
			IndexReader*		_OpenIndexReader();

			bool				_RemoveDocuments(std::vector<entry_ref>& docs);
			bool				_RemoveDocument(wchar_t* doc,
									IndexReader* reader);

			bool				_IndexDocument(const entry_ref& ref);

			BPath				fDataBasePath;

			BPath				fTempPath;

			std::vector<entry_ref>	fAddQueue;
			std::vector<entry_ref>	fDeleteQueue;

			StandardAnalyzer	fStandardAnalyzer;

			IndexWriter*		fIndexWriter;
};

#endif
