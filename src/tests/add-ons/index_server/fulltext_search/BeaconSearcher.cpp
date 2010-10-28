/*
 * Copyright 2009-2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ankur Sethi (get.me.ankur@gmail.com)
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "BeaconSearcher.h"

#include <cstring>

#include <Alert.h>
#include <VolumeRoster.h>

#include "IndexServerPrivate.h"


using namespace lucene::document ;
using namespace lucene::search ;
using namespace lucene::index ;
using namespace lucene::queryParser ;


BeaconSearcher::BeaconSearcher()
{
	BVolumeRoster volumeRoster ;
	BVolume volume ;
	IndexSearcher *indexSearcher ;

	while(volumeRoster.GetNextVolume(&volume) == B_OK) {
		BPath indexPath = GetIndexPath(&volume);
		if(IndexReader::indexExists(indexPath.Path())) {
			indexSearcher = new IndexSearcher(indexPath.Path());
			fSearcherList.AddItem(indexSearcher);
		}
	}
}


BeaconSearcher::~BeaconSearcher()
{
	IndexSearcher *indexSearcher;
	while(fSearcherList.CountItems() > 0) {
		indexSearcher = (IndexSearcher*)fSearcherList.ItemAt(0);
		indexSearcher->close();
		delete indexSearcher;
		fSearcherList.RemoveItem((int32)0);
	}
}


BPath
BeaconSearcher::GetIndexPath(BVolume *volume)
{
	BDirectory dir;
	volume->GetRootDirectory(&dir);
	BPath path(&dir);
	path.Append(kIndexServerDirectory);
	path.Append("FullTextAnalyser");
	
	return path;
}


void
BeaconSearcher::Search(const char* stringQuery)
{
	// CLucene expects wide characters everywhere.
	int size = strlen(stringQuery) * sizeof(wchar_t) ;
	wchar_t *wStringQuery = new wchar_t[size] ;
	if (mbstowcs(wStringQuery, stringQuery, size) == -1)
		return ;

	IndexSearcher *indexSearcher ;
	Hits *hits ;
	Query *luceneQuery ;
	Document doc ;
	Field *field ;
	wchar_t *path ;
	
	/*
	luceneQuery = QueryParser::parse(wStringQuery, _T("contents"),
		&fStandardAnalyzer) ;

	hits = fMultiSearcher->search(luceneQuery) ;
	for(int j = 0 ; j < hits->length() ; j++) {
		doc = hits->doc(j) ;
		field = doc.getField(_T("path")) ;
		path = new wchar_t[B_PATH_NAME_LENGTH * sizeof(wchar_t)] ;
		wcscpy(path, field->stringValue()) ;
		fHits.AddItem(path) ;
	}*/
	
	for(int i = 0 ; (indexSearcher = (IndexSearcher*)fSearcherList.ItemAt(i))
		!= NULL ; i++) {
		luceneQuery = QueryParser::parse(wStringQuery, _T("contents"),
			&fStandardAnalyzer) ;

		hits = indexSearcher->search(luceneQuery) ;

		for(int j = 0 ; j < hits->length() ; j++) {
			doc = hits->doc(j) ;
			field = doc.getField(_T("path")) ;
			path = new wchar_t[B_PATH_NAME_LENGTH * sizeof(wchar_t)] ;
			wcscpy(path, field->stringValue()) ;
			fHits.AddItem(path) ;
		}
	}
}


wchar_t*
BeaconSearcher::GetNextHit()
{
	if(fHits.CountItems() != 0) {
		wchar_t* path = (wchar_t*)fHits.ItemAt(0) ;
		fHits.RemoveItem((int32)0) ;
		return path ;
	}
	
	return NULL ;
}

