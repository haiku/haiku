//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file intel_parameters.h
	\brief Class interface definitions for "intel" style partitioning
		   parameter support.
*/

#ifndef _INTEL_PARAMETERS_H
#define _INTEL_PARAMETERS_H

#include <SupportDefs.h>

class Partition;
class PartitionMap;
class PrimaryPartition;

// tokens
enum {
	TOKEN_ERROR		= -1,
	TOKEN_EOF		= '\0',
	TOKEN_OPEN		= '{',
	TOKEN_CLOSE		= '}',
	TOKEN_COMMA		= ',',
	TOKEN_NUMBER	= '0',
};

// Tokenizer
class Tokenizer {
public:
	Tokenizer();
	Tokenizer(const char *input);

	void SetTo(const char *input);

	int32 GetNextToken(char *buffer = NULL);
	void PutLastToken() { fPosition = fLastPosition; }

	bool ExpectToken(int32 kind, char *buffer = NULL);

	bool ReadOpen()		{ return (ExpectToken(TOKEN_OPEN)); }
	bool ReadClose()	{ return (ExpectToken(TOKEN_CLOSE)); }
	bool ReadComma()	{ return (ExpectToken(TOKEN_COMMA)); }
	bool ReadEOF()		{ return (ExpectToken(TOKEN_EOF)); }
	bool ReadNumber(int64 &number);

private:
	void _SkipWhiteSpace();

private:
	const char	*fInput;
	int32		fPosition;
	int32		fLastPosition;
};

// ParameterParser
class ParameterParser {
public:
	ParameterParser();
	~ParameterParser() {}

	status_t Parse(const char *parameters, PartitionMap *map);

private:
	bool _ParsePrimaryPartition(PrimaryPartition *primary);
	bool _ParsePartition(Partition *partition);
	void _ErrorOccurred(status_t error = B_ERROR);
	bool _NoError() const	{ return (fParseError == B_OK); }

private:
	Tokenizer		fTokenizer;
	status_t		fParseError;
	PartitionMap	*fMap;
};

// ParameterUnparser
class ParameterUnparser {
public:
	ParameterUnparser();
	~ParameterUnparser() {}

	status_t GetParameterLength(const PartitionMap *map, size_t *length);
	status_t Unparse(const PartitionMap *map, char *parameters, size_t length);

private:
	bool _UnparsePartitionMap();
	bool _UnparsePrimaryPartition(const PrimaryPartition *primary);
	bool _UnparsePartition(const Partition *partition);

	bool _Write(const char *str);
	bool _WriteOpen()	{ return _Write("{"); }
	bool _WriteClose()	{ return _Write("}"); }
	bool _WriteComma()	{ return _Write(","); }
	bool _WriteNumber(int64 number);

	void _ErrorOccurred(status_t error = B_ERROR);
	bool _NoError() const	{ return (fUnparseError == B_OK); }

private:
	const PartitionMap	*fMap;
	char				*fParameters;
	size_t				fPosition;
	size_t				fSize;
	bool				fDryRun;
	status_t			fUnparseError;
};

#endif	// _INTEL_PARAMETERS_H
