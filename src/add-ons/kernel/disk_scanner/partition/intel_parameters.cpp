//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file intel_parameters.cpp
	\brief Class implementations for "intel" style partitioning
		   parameter support.
*/

#include <ctype.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intel_parameters.h"
#include "intel_partition_map.h"

// Grammar for parameters:
//
//	partition		::= "{" ptsoffset "," offset "," size "," type ","
//						active "}"
//	primary			::= "{" partition+ "}"
//	partition_map	::= primary primary primary primary

// longest token (including terminating null)
const int32 kMaxTokenLen = 256;


// Tokenizer

// constructor
Tokenizer::Tokenizer()
	: fInput(NULL),
	  fPosition(0),
	  fLastPosition(0)
{
}

// constructor
Tokenizer::Tokenizer(const char *input)
	: fInput(input),
	  fPosition(0),
	  fLastPosition(0)
{
}

// SetTo
void
Tokenizer::SetTo(const char *input)
{
	fInput = input;
	fPosition = 0;
	fLastPosition = 0;
}

// GetNextToken
int32
Tokenizer::GetNextToken(char *buffer)
{
	int32 kind = TOKEN_ERROR;
	fLastPosition = fPosition;
	// skip WS
	_SkipWhiteSpace();
	switch (fInput[fPosition]) {
		case '{':
		case '}':
		case ',':
			kind = fInput[fPosition];
			fPosition++;
			break;
		case '\0':
			kind = TOKEN_EOF;
			break;
		default:
		{
			if (isdigit(fInput[fPosition])) {
				int32 startPos = fPosition;
				while (isdigit(fInput[fPosition]))
					fPosition++;
				int32 len = fPosition - startPos;
				if (len < kMaxTokenLen) {
					if (buffer) {
						memcpy(buffer, fInput + startPos, len);
						buffer[len] = '\0';
					}
					kind = TOKEN_NUMBER;
				} else {
					printf("token too long at: %ld\n", startPos);
					kind = TOKEN_ERROR;
				}
			}
			break;
		}
	}
	return kind;
}

// ExpectToken
bool
Tokenizer::ExpectToken(int32 kind, char *buffer)
{
	bool result = (GetNextToken(buffer) == kind);
	if (!result)
		PutLastToken();
	return result;
}

// ReadNumber
bool
Tokenizer::ReadNumber(int64 &number)
{
	char buffer[kMaxTokenLen];
	bool result = ExpectToken(TOKEN_NUMBER, buffer);
	if (result)
		number = atoll(buffer);
	return result;
}

// _SkipWhiteSpace
void
Tokenizer::_SkipWhiteSpace()
{
	while (fInput[fPosition] != '\0' && isspace(fInput[fPosition]))
		fPosition++;
}


// ParameterParser

// constructor
ParameterParser::ParameterParser()
	: fTokenizer(),
	  fParseError(B_OK),
	  fMap(NULL)
{
}

// Parse
status_t
ParameterParser::Parse(const char *parameters, PartitionMap *map)
{
	status_t error = (parameters && map ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// init parser
		fParseError = B_OK;
		fTokenizer.SetTo(parameters);
		fMap = map;
		// partition_map	::= primary primary primary primary
		if (_ParsePrimaryPartition(fMap->PrimaryPartitionAt(0))
			&& _ParsePrimaryPartition(fMap->PrimaryPartitionAt(1))
			&& _ParsePrimaryPartition(fMap->PrimaryPartitionAt(2))
			&& _ParsePrimaryPartition(fMap->PrimaryPartitionAt(3))
			&& fTokenizer.ReadEOF()) {
			// successfully parsed
		} else
			_ErrorOccurred();
		// cleanup / set results
		fMap = NULL;
		error = fParseError;
	}
	return error;
}

// _ParsePrimaryPartition
bool
ParameterParser::_ParsePrimaryPartition(PrimaryPartition *primary)
{
	// primary			::= "{" partition+ "}"
	if (fTokenizer.ReadOpen()
		&& _ParsePartition(primary)) {
		while (_NoError() && !fTokenizer.ReadClose()) {
			LogicalPartition *partition = new(nothrow) LogicalPartition;
			if (partition) {
				if (_ParsePartition(partition))
					primary->AddLogicalPartition(partition);
				else
					delete partition;
			} else
				_ErrorOccurred(B_NO_MEMORY);
		}
	} else
		_ErrorOccurred();
	return _NoError();
}

// _ParsePartition
bool
ParameterParser::_ParsePartition(Partition *partition)
{
	//	partition		::= "{" ptsoffset "," offset "," size "," type ","
	//						active "}"
	int64 ptsOffset, offset, size, type, active;
	if (fTokenizer.ReadOpen()
		&& fTokenizer.ReadNumber(ptsOffset)
		&& fTokenizer.ReadComma()
		&& fTokenizer.ReadNumber(offset)
		&& fTokenizer.ReadComma()
		&& fTokenizer.ReadNumber(size)
		&& fTokenizer.ReadComma()
		&& fTokenizer.ReadNumber(type)
		&& fTokenizer.ReadComma()
		&& fTokenizer.ReadNumber(active)
		&& fTokenizer.ReadClose()) {
		if (ptsOffset >= 0 && offset >= 0 && size >= 0
			&& type >= 0 && type < 256) {
			partition->SetPTSOffset(ptsOffset);
			partition->SetOffset(offset);
			partition->SetSize(size);
			partition->SetType((uint8)type);
			partition->SetActive(active);
		} else
			_ErrorOccurred(B_BAD_VALUE);
	} else
		_ErrorOccurred();
	return _NoError();
}

// _ErrorOccurred
void
ParameterParser::_ErrorOccurred(status_t error)
{
	if (fParseError == B_OK)
		fParseError = error;
}


// ParameterUnparser

// constructor
ParameterUnparser::ParameterUnparser()
	: fMap(NULL),
	  fParameters(NULL),
	  fPosition(0),
	  fSize(0),
	  fDryRun(false),
	  fUnparseError(B_OK)
{
}

// GetParameterLength
status_t
ParameterUnparser::GetParameterLength(const PartitionMap *map, size_t *length)
{
	status_t error = (map && length ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// init unparser for a dry run
		fUnparseError = B_OK;
		fMap = map;
		char buffer[kMaxTokenLen];
		fParameters = buffer;
		fPosition = 0;
		fSize = 0;
		fDryRun = true;
		// dry run -- get the parameter size
		if (_UnparsePartitionMap())
			*length = fPosition + 1;
		// cleanup / set results
		fMap = NULL;
		error = fUnparseError;
	}
	return error;
}

// Unparse
status_t
ParameterUnparser::Unparse(const PartitionMap *map, char *parameters,
						   size_t size)
{
	status_t error = (map && parameters ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// init unparser
		fUnparseError = B_OK;
		fMap = map;
		fParameters = parameters;
		fPosition = 0;
		fSize = size;
		fDryRun = false;
		// unparse the partition map
		_UnparsePartitionMap();
		// cleanup / set results
		fMap = NULL;
		error = fUnparseError;
	}
	return error;
}

// _UnparsePartitionMap
bool
ParameterUnparser::_UnparsePartitionMap()
{
	if (_UnparsePrimaryPartition(fMap->PrimaryPartitionAt(0))
		&& _UnparsePrimaryPartition(fMap->PrimaryPartitionAt(1))
		&& _UnparsePrimaryPartition(fMap->PrimaryPartitionAt(2))
		&& _UnparsePrimaryPartition(fMap->PrimaryPartitionAt(3))) {
		// successfully unparsed
	}
	return _NoError();
}

// _UnparsePrimaryPartition
bool
ParameterUnparser::_UnparsePrimaryPartition(const PrimaryPartition *primary)
{
	// primary			::= "{" partition+ "}"
	if (_WriteOpen() && _UnparsePartition(primary)) {
		for (int32 i = 0; i < primary->CountLogicalPartitions(); i++) {
			const LogicalPartition *partition = primary->LogicalPartitionAt(i);
			if (!_UnparsePartition(partition))
				break;
		}
		if (_NoError())
			_WriteClose();
	}
	return _NoError();
}

// _UnparsePartition
bool
ParameterUnparser::_UnparsePartition(const Partition *partition)
{
	//	partition		::= "{" ptsoffset "," offset "," size "," type ","
	//						active "}"
	if (_WriteOpen()
		&& _WriteNumber(partition->PTSOffset())
		&& _WriteComma()
		&& _WriteNumber(partition->Offset())
		&& _WriteComma()
		&& _WriteNumber(partition->Size())
		&& _WriteComma()
		&& _WriteNumber(partition->Type())
		&& _WriteComma()
		&& _WriteNumber(partition->Active() ? 1 : 0)
		&& _WriteClose()) {
		// success
	}
	return _NoError();
}

// _Write
bool
ParameterUnparser::_Write(const char *str)
{
	size_t len = strlen(str);
	if (!fDryRun) {
		if (fSize > fPosition + len)
			strcpy(fParameters + fPosition, str);
		else
			_ErrorOccurred(B_BAD_VALUE);
	}
	fPosition += len;
	return _NoError();
}

// _WriteNumber
bool
ParameterUnparser::_WriteNumber(int64 number)
{
	char buffer[kMaxTokenLen];
	sprintf(buffer, "%lld", number);
	return _Write(buffer);
}

// _ErrorOccurred
void
ParameterUnparser::_ErrorOccurred(status_t error)
{
	if (fUnparseError == B_OK)
		fUnparseError = error;
}



/*
// main
int
main()
{
	const char *parameters = "{{ 0, 10,  1, 0}{ 0, 3,  1, 0}}"
							 "{{10, 13,  1, 1}}"
							 "{{23,  7,  1, 0}}"
							 "{{30,  9,  1, 0}}";
	ParameterParser parser;
	status_t error = parser.Parse(parameters);
	if (error != B_OK)
		printf("error while parsing: %s\n", strerror(error));
	return 0;
}
*/
