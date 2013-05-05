/*
 * Copyright 2007-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ArgumentVector.h>

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>


struct ArgumentVector::Parser {
	ParseError Parse(const char* commandLine, const char*& _errorLocation)
	{
		// init temporary arg/argv storage
		fCurrentArg.clear();
		fCurrentArgStarted = false;
		fArgVector.clear();
		fTotalStringSize = 0;

		for (; *commandLine; commandLine++) {
			char c = *commandLine;

			// whitespace delimits args and is otherwise ignored
			if (isspace(c)) {
				_PushCurrentArg();
				continue;
			}

			const char* errorBase = commandLine;

			switch (c) {
				case '\'':
					// quoted string -- no quoting
					while (*++commandLine != '\'') {
						c = *commandLine;
						if (c == '\0') {
							_errorLocation = errorBase;
							return UNTERMINATED_QUOTED_STRING;
						}
						_PushCharacter(c);
					}
					break;

				case '"':
					// quoted string -- some quoting
					while (*++commandLine != '"') {
						c = *commandLine;
						if (c == '\0') {
							_errorLocation = errorBase;
							return UNTERMINATED_QUOTED_STRING;
						}

						if (c == '\\') {
							c = *++commandLine;
							if (c == '\0') {
								_errorLocation = errorBase;
								return UNTERMINATED_QUOTED_STRING;
							}

							// only '\' and '"' can be quoted, otherwise the
							// the '\' is treated as a normal char
							if (c != '\\' && c != '"')
								_PushCharacter('\\');
						}

						_PushCharacter(c);
					}
					break;

				case '\\':
					// quoted char
					c = *++commandLine;
					if (c == '\0') {
						_errorLocation = errorBase;
						return TRAILING_BACKSPACE;
					}
					_PushCharacter(c);
					break;

				default:
					// normal char
					_PushCharacter(c);
					break;
			}
		}

		// commit last arg
		_PushCurrentArg();

		return NO_ERROR;
	}

	const std::vector<std::string>& ArgVector() const
	{
		return fArgVector;
	}

	size_t TotalStringSize() const
	{
		return fTotalStringSize;
	}

private:
	void _PushCurrentArg()
	{
		if (fCurrentArgStarted) {
			fArgVector.push_back(fCurrentArg);
			fTotalStringSize += fCurrentArg.length() + 1;
			fCurrentArgStarted = false;
		}
	}

	void _PushCharacter(char c)
	{
		if (!fCurrentArgStarted) {
			fCurrentArg = "";
			fCurrentArgStarted = true;
		}

		fCurrentArg += c;
	}

private:
	// temporaries
	std::string					fCurrentArg;
	bool						fCurrentArgStarted;
	std::vector<std::string>	fArgVector;
	size_t						fTotalStringSize;
};


ArgumentVector::ArgumentVector()
	:
	fArguments(NULL),
	fCount(0)
{
}


ArgumentVector::~ArgumentVector()
{
	free(fArguments);
}


char**
ArgumentVector::DetachArguments()
{
	char** arguments = fArguments;
	fArguments = NULL;
	fCount = 0;
	return arguments;
}


ArgumentVector::ParseError
ArgumentVector::Parse(const char* commandLine, const char** _errorLocation)
{
	free(DetachArguments());

	ParseError error;
	const char* errorLocation = commandLine;

	try {
		Parser parser;
		error = parser.Parse(commandLine, errorLocation);

		if (error == NO_ERROR) {
			// Create a char* array and copy everything into a single
			// allocation.
			int count = parser.ArgVector().size();
			size_t arraySize = (count + 1) * sizeof(char*);
			fArguments = (char**)malloc(
				arraySize + parser.TotalStringSize());
			if (fArguments != 0) {
				char* argument = (char*)(fArguments + count + 1);
				for (int i = 0; i < count; i++) {
					fArguments[i] = argument;
					const std::string& sourceArgument = parser.ArgVector()[i];
					size_t argumentSize = sourceArgument.length() + 1;
					memcpy(argument, sourceArgument.c_str(), argumentSize);
					argument += argumentSize;
				}

				fArguments[count] = NULL;
				fCount = count;
			} else
				error = NO_MEMORY;
		}
	} catch (...) {
		error = NO_MEMORY;
	}

	if (error != NO_ERROR && _errorLocation != NULL)
		*_errorLocation = errorLocation;

	return error;
}
