/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CLanguageFamilySyntaxHighlightInfo.h"

#include <AutoDeleter.h>

#include "CLanguageTokenizer.h"
#include "LineDataSource.h"
#include "TeamTypeInformation.h"
#include "TypeLookupConstraints.h"


using namespace CLanguage;


static const char* kLanguageKeywords[] = {
	"NULL",
	"asm",
	"auto",
	"bool",
	"break",
	"case",
	"catch",
	"char",
	"class",
	"const",
	"const_cast",
	"constexpr",
	"continue",
	"default",
	"delete",
	"do",
	"double",
	"dynamic_cast",
	"else",
	"enum",
	"explicit",
	"extern",
	"false",
	"float",
	"for",
	"goto",
	"if",
	"inline",
	"int",
	"long",
	"mutable",
	"namespace",
	"new",
	"operator",
	"private",
	"protected",
	"public",
	"register",
	"reinterpret_cast",
	"return",
	"short",
	"signed",
	"sizeof",
	"static",
	"static_cast",
	"struct",
	"switch",
	"template",
	"this",
	"throw",
	"true",
	"try",
	"typedef",
	"typeid",
	"typename",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile",
	"while"
};


static bool IsLanguageKeyword(const Token& token)
{
	int lower = 0;
	int upper = (sizeof(kLanguageKeywords)/sizeof(char*)) - 1;

	while (lower < upper) {
		int mid = (lower + upper + 1) / 2;

		int cmp = token.string.Compare(kLanguageKeywords[mid]);
		if (cmp == 0)
			return true;
		else if (cmp < 0)
			upper = mid - 1;
		else
			lower = mid;
	}

	return token.string.Compare(kLanguageKeywords[lower]) == 0;
}


// #pragma mark - CLanguageFamilySyntaxHighlightInfo::SyntaxPair


struct CLanguageFamilySyntaxHighlightInfo::SyntaxPair {
	int32 column;
	syntax_highlight_type type;

	SyntaxPair(int32 column, syntax_highlight_type type)
		:
		column(column),
		type(type)
	{
	}
};


// #pragma mark - CLanguageFamilySyntaxHighlightInfo::LineInfo


class CLanguageFamilySyntaxHighlightInfo::LineInfo {
public:
	LineInfo(int32 line)
		:
		fLine(line),
		fPairs(5, true)
	{
	}

	inline int32 CountPairs() const
	{
		return fPairs.CountItems();
	}

	SyntaxPair* PairAt(int32 index) const
	{
		return fPairs.ItemAt(index);
	}

	bool AddPair(int32 column, syntax_highlight_type type)
	{
		SyntaxPair* pair = new(std::nothrow) SyntaxPair(column, type);
		if (pair == NULL)
			return false;

		ObjectDeleter<SyntaxPair> pairDeleter(pair);
		if (!fPairs.AddItem(pair))
			return false;

		pairDeleter.Detach();
		return true;
	}

private:
	typedef BObjectList<SyntaxPair> SyntaxPairList;

private:
	int32 fLine;
	SyntaxPairList fPairs;
};


// #pragma mark - CLanguageFamilySyntaxHighlightInfo;


CLanguageFamilySyntaxHighlightInfo::CLanguageFamilySyntaxHighlightInfo(
	LineDataSource* source, Tokenizer* tokenizer,
	TeamTypeInformation* typeInfo)
	:
	SyntaxHighlightInfo(),
	fHighlightSource(source),
	fTokenizer(tokenizer),
	fTypeInfo(typeInfo),
	fLineInfos(10, true)
{
	fHighlightSource->AcquireReference();
}


CLanguageFamilySyntaxHighlightInfo::~CLanguageFamilySyntaxHighlightInfo()
{
	fHighlightSource->ReleaseReference();
	delete fTokenizer;
}


int32
CLanguageFamilySyntaxHighlightInfo::GetLineHighlightRanges(int32 line,
	int32* _columns, syntax_highlight_type* _types, int32 maxCount)
{
	if (line >= fHighlightSource->CountLines())
		return 0;

	// lazily parse the source's highlight information the first time
	// it's actually requested. Subsequently it's cached for quick retrieval.
	if (fLineInfos.CountItems() == 0) {
		if (_ParseLines() != B_OK)
			return 0;
	}

	LineInfo* info = fLineInfos.ItemAt(line);
	if (info == NULL)
		return 0;

	int32 count = 0;
	for (; count < info->CountPairs(); count++) {
		if (count == maxCount - 1)
			break;

		SyntaxPair* pair = info->PairAt(count);
		if (pair == NULL)
			break;

		_columns[count] = pair->column;
		_types[count] = pair->type;
	}

	return count;
}


status_t
CLanguageFamilySyntaxHighlightInfo::_ParseLines()
{
	syntax_highlight_type type = SYNTAX_HIGHLIGHT_NONE;

	for (int32 i = 0; i < fHighlightSource->CountLines(); i++) {
		const char* line = fHighlightSource->LineAt(i);
		fTokenizer->SetTo(line);
		LineInfo* info = NULL;

		status_t error = _ParseLine(i, type, info);
		if (error != B_OK)
			return error;

		ObjectDeleter<LineInfo> infoDeleter(info);
		if (!fLineInfos.AddItem(info))
			return B_NO_MEMORY;

		infoDeleter.Detach();
	}

	return B_OK;
}


status_t
CLanguageFamilySyntaxHighlightInfo::_ParseLine(int32 line,
	syntax_highlight_type& _lastType, LineInfo*& _info)
{
	bool inCommentBlock = (_lastType == SYNTAX_HIGHLIGHT_COMMENT);
	bool inPreprocessor = false;

	_info = new(std::nothrow) LineInfo(line);
	if (_info == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<LineInfo> infoDeleter(_info);
	if (inCommentBlock) {
		if (!_info->AddPair(0, SYNTAX_HIGHLIGHT_COMMENT))
			return B_NO_MEMORY;
	}

	try {
		for (;;) {
			const Token& token = fTokenizer->NextToken();
			if (token.type == TOKEN_END_OF_LINE)
				break;

			if (inCommentBlock) {
				if (token.type == TOKEN_END_COMMENT_BLOCK)
					inCommentBlock = false;
				continue;
			} else if (inPreprocessor) {
				fTokenizer->NextToken();
				inPreprocessor = false;
			} else if (token.type == TOKEN_INLINE_COMMENT) {
				if (!_info->AddPair(token.position, SYNTAX_HIGHLIGHT_COMMENT))
					return B_NO_MEMORY;
				break;
			}

			syntax_highlight_type current = _MapTokenToSyntaxType(token);
			if (_lastType == current)
				continue;

			_lastType = current;
			if (!_info->AddPair(token.position, current))
				return B_NO_MEMORY;

			if (token.type == TOKEN_BEGIN_COMMENT_BLOCK)
				inCommentBlock = true;
			else if (token.type == TOKEN_POUND)
				inPreprocessor = true;
		}
	} catch (...) {
		// if a parse exception was thrown, simply ignore it.
		// in such a case, we can't guarantee correct highlight
		// information anyhow, so simply return whatever we started
		// with.
	}

	_lastType = inCommentBlock
		? SYNTAX_HIGHLIGHT_COMMENT : SYNTAX_HIGHLIGHT_NONE;
	infoDeleter.Detach();
	return B_OK;
}


syntax_highlight_type
CLanguageFamilySyntaxHighlightInfo::_MapTokenToSyntaxType(const Token& token)
{
	static TypeLookupConstraints constraints;

	switch (token.type) {
		case TOKEN_IDENTIFIER:
			if (IsLanguageKeyword(token))
				return SYNTAX_HIGHLIGHT_KEYWORD;
			else if (fTypeInfo->TypeExistsByName(token.string, constraints))
				return SYNTAX_HIGHLIGHT_TYPE;
			break;

		case TOKEN_CONSTANT:
			return SYNTAX_HIGHLIGHT_NUMERIC_LITERAL;

		case TOKEN_END_OF_LINE:
			break;

		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_STAR:
		case TOKEN_SLASH:
		case TOKEN_MODULO:
		case TOKEN_OPENING_PAREN:
		case TOKEN_CLOSING_PAREN:
		case TOKEN_OPENING_SQUARE_BRACKET:
		case TOKEN_CLOSING_SQUARE_BRACKET:
		case TOKEN_OPENING_CURLY_BRACE:
		case TOKEN_CLOSING_CURLY_BRACE:
		case TOKEN_LOGICAL_AND:
		case TOKEN_LOGICAL_OR:
		case TOKEN_LOGICAL_NOT:
		case TOKEN_BITWISE_AND:
		case TOKEN_BITWISE_OR:
		case TOKEN_BITWISE_NOT:
		case TOKEN_BITWISE_XOR:
		case TOKEN_EQ:
		case TOKEN_NE:
		case TOKEN_GT:
		case TOKEN_GE:
		case TOKEN_LT:
		case TOKEN_LE:
		case TOKEN_MEMBER_PTR:
		case TOKEN_CONDITION:
		case TOKEN_COLON:
		case TOKEN_SEMICOLON:
		case TOKEN_BACKSLASH:
			return SYNTAX_HIGHLIGHT_OPERATOR;

		case TOKEN_STRING_LITERAL:
			return SYNTAX_HIGHLIGHT_STRING_LITERAL;

		case TOKEN_POUND:
			return SYNTAX_HIGHLIGHT_PREPROCESSOR_KEYWORD;

		case TOKEN_BEGIN_COMMENT_BLOCK:
		case TOKEN_END_COMMENT_BLOCK:
		case TOKEN_INLINE_COMMENT:
			return SYNTAX_HIGHLIGHT_COMMENT;
	}

	return SYNTAX_HIGHLIGHT_NONE;
}
