/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		based on previous work of Ankur Sethi
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "CLuceneDataBase.h"

#include <Directory.h>
#include <File.h>
#include <TranslatorRoster.h>


#define DEBUG_CLUCENE_DATABASE
#ifdef DEBUG_CLUCENE_DATABASE
#include <stdio.h>
#	define STRACE(x...) printf("FT: " x)
#else
#	define STRACE(x...) ;
#endif


using namespace lucene::document;
using namespace lucene::util;


const uint8 kCluceneTries = 10;


wchar_t* to_wchar(const char *str)
{
	int size = strlen(str) * sizeof(wchar_t) ;
	wchar_t *wStr = new wchar_t[size] ;

	if (mbstowcs(wStr, str, size) == -1)
		return NULL ;
	else
		return wStr ;
}


CLuceneWriteDataBase::CLuceneWriteDataBase(const BPath& databasePath)
	:
	fDataBasePath(databasePath),
	fTempPath(databasePath),
	fIndexWriter(NULL)
{
	printf("CLuceneWriteDataBase fDataBasePath %s\n", fDataBasePath.Path());
	create_directory(fDataBasePath.Path(), 0755);

	fTempPath.Append("temp_file");
}


CLuceneWriteDataBase::~CLuceneWriteDataBase()
{
	// TODO: delete fTempPath file
}


status_t
CLuceneWriteDataBase::InitCheck()
{

	return B_OK;
}


status_t
CLuceneWriteDataBase::AddDocument(const entry_ref& ref)
{
	// check if already in the queue
	for (unsigned int i = 0; i < fAddQueue.size(); i++) {
		if (fAddQueue.at(i) == ref)
			return B_OK;
	}
	fAddQueue.push_back(ref);

	return B_OK;
}


status_t
CLuceneWriteDataBase::RemoveDocument(const entry_ref& ref)
{
	// check if already in the queue
	for (unsigned int i = 0; i < fAddQueue.size(); i++) {
		if (fDeleteQueue.at(i) == ref)
			return B_OK;
	}
	fDeleteQueue.push_back(ref);
	return B_OK;
}


status_t
CLuceneWriteDataBase::Commit()
{
	if (fAddQueue.size() == 0 && fDeleteQueue.size() == 0)
		return B_OK;
	STRACE("Commit\n");

	_RemoveDocuments(fAddQueue);
	_RemoveDocuments(fDeleteQueue);
	fDeleteQueue.clear();

	if (fAddQueue.size() == 0)
		return B_OK;

	fIndexWriter = _OpenIndexWriter();
	if (fIndexWriter == NULL)
		return B_ERROR;

	status_t status = B_OK;
	for (unsigned int i = 0; i < fAddQueue.size(); i++) {
		if (!_IndexDocument(fAddQueue.at(i))) {
			status = B_ERROR;
			break;
		}
	}

	fAddQueue.clear();
	fIndexWriter->close();
	delete fIndexWriter;
	fIndexWriter = NULL;

	return status;
}


IndexWriter*
CLuceneWriteDataBase::_OpenIndexWriter()
{
	IndexWriter* writer = NULL;
	for (int i = 0; i < kCluceneTries; i++) {
		try {
			bool createIndex = true;
			if (IndexReader::indexExists(fDataBasePath.Path()))
				createIndex = false;

			writer = new IndexWriter(fDataBasePath.Path(),
				&fStandardAnalyzer, createIndex);
			if (writer)
				break;
		} catch (CLuceneError &error) {
			STRACE("CLuceneError: _OpenIndexWriter %s\n", error.what());
			delete writer;
			writer = NULL;
		}
	}
	return writer;
}


IndexReader*
CLuceneWriteDataBase::_OpenIndexReader()
{
	IndexReader* reader = NULL;

	BEntry entry(fDataBasePath.Path(), NULL);
	if (!entry.Exists())
		return NULL;

	for (int i = 0; i < kCluceneTries; i++) {
		try {
			if (!IndexReader::indexExists(fDataBasePath.Path()))
				return NULL;

			reader = IndexReader::open(fDataBasePath.Path());
			if (reader)
				break;
		} catch (CLuceneError &error) {
			STRACE("CLuceneError: _OpenIndexReader %s\n", error.what());
			delete reader;
			reader = NULL;
		}
	}

	return reader;
}


bool
CLuceneWriteDataBase::_RemoveDocuments(std::vector<entry_ref>& docs)
{
	IndexReader *reader = NULL;
	reader = _OpenIndexReader();
	if (!reader)
		return false;
	bool status = false;

	for (unsigned int i = 0; i < docs.size(); i++) {
		BPath path(&docs.at(i));
		wchar_t* wPath = to_wchar(path.Path());
		if (wPath == NULL)
			continue;
		
		for (int i = 0; i < kCluceneTries; i++) {
			status = _RemoveDocument(wPath, reader);
			if (status)
				break;
			reader->close();
			delete reader;
			reader = _OpenIndexReader();
			if (!reader) {
				status = false;
				break;
			}
		}
		delete wPath;

		if (!status)
			break;
	}

	reader->close();
	delete reader;

	return status;
}


bool
CLuceneWriteDataBase::_RemoveDocument(wchar_t* wPath, IndexReader* reader)
{
	try {
		Term term(_T("path"), wPath);
		reader->deleteDocuments(&term);
	} catch (CLuceneError &error) {
		STRACE("CLuceneError: deleteDocuments %s\n", error.what());
		return false;
	}
	return true;
}


bool
CLuceneWriteDataBase::_IndexDocument(const entry_ref& ref)
{
	BPath path(&ref);

	BFile inFile, outFile;
	inFile.SetTo(path.Path(), B_READ_ONLY);
	if (inFile.InitCheck() != B_OK) {
		STRACE("Can't open inFile %s\n", path.Path());
		return false;
	}
	outFile.SetTo(fTempPath.Path(),
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (outFile.InitCheck() != B_OK) {
		STRACE("Can't open outFile %s\n", fTempPath.Path());
		return false;
	}

	BTranslatorRoster* translatorRoster = BTranslatorRoster::Default();
	if (translatorRoster->Translate(&inFile, NULL, NULL, &outFile, 'TEXT')
		!= B_OK)
		return false;

	inFile.Unset(); 
	outFile.Unset();

	FileReader* fileReader = new FileReader(fTempPath.Path(), "UTF-8");
	wchar_t* wPath = to_wchar(path.Path());
	if (wPath == NULL)
		return false;

	Document *document = new Document;
	Field contentField(_T("contents"), fileReader,
		Field::STORE_NO | Field::INDEX_TOKENIZED);
	document->add(contentField);
	Field pathField(_T("path"), wPath,
		Field::STORE_YES | Field::INDEX_UNTOKENIZED);
	document->add(pathField);

	bool status = true;
	for (int i = 0; i < kCluceneTries; i++) {
		try {
			fIndexWriter->addDocument(document);
			STRACE("document added, retries: %i\n", i);
			break;
		} catch (CLuceneError &error) {
			STRACE("CLuceneError addDocument %s\n", error.what());
			fIndexWriter->close();
			delete fIndexWriter;
			fIndexWriter = _OpenIndexWriter();
			if (fIndexWriter == NULL) {
				status = false;
				break;
			}
		}
	}

	if (!status)
		delete document;
	delete wPath;
	return status;
}
