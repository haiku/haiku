/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H
#define C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H


#include "SyntaxHighlighter.h"

#include <ObjectList.h>


namespace CLanguage {
	struct Token;
	class Tokenizer;
}

class TeamTypeInformation;


class CLanguageFamilySyntaxHighlightInfo : public SyntaxHighlightInfo {
public:
								CLanguageFamilySyntaxHighlightInfo(
									LineDataSource* source,
									CLanguage::Tokenizer* tokenizer,
									TeamTypeInformation* info);
	virtual						~CLanguageFamilySyntaxHighlightInfo();

	virtual	int32				GetLineHighlightRanges(int32 line,
									int32* _columns,
									syntax_highlight_type* _types,
									int32 maxCount);

private:
	class LineInfo;
	typedef BObjectList<LineInfo> LineInfoList;
	struct SyntaxPair;

private:
			status_t			_ParseLines();
			status_t			_ParseLine(int32 line,
									syntax_highlight_type& _lastType,
									LineInfo*& _info);
			syntax_highlight_type _MapTokenToSyntaxType(
									const CLanguage::Token& token);
private:
	LineDataSource*				fHighlightSource;
	CLanguage::Tokenizer*		fTokenizer;
	TeamTypeInformation*		fTypeInfo;
	LineInfoList				fLineInfos;
};


#endif	// C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H
