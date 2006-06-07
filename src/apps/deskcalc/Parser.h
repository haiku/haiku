/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 2004 Daniel Wallner. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Daniel Wallner <daniel.wallner@bredband.net>
 */

#ifndef PARSER_H
#define PARSER_H

#include <vector.h>
#include <string>
#include <stdio.h>

//class LineParser {
// public:
//  LineParser(std::istream *stream);
//
//  void AddSeparator(const char c);
//
//  size_t SetToNext(bool toUpperCase = false);
//  void WriteCompressed(std::string *s) const;
//  void WriteCompressed(std::ostream *stream) const;
//
//  size_t ParsedLines() const;
//  const char *Param(const size_t param) const;
//  const char *Line() const;
//
// private:
//  LineParser();
//
//  std::istream *m_stream;
//  size_t m_currentLine;
//  std::string m_unparsed;
//  std::vector<std::string> m_line;
//  std::vector<char> m_separators;
//};

class Expression {
 public:
								Expression();

			void				SetAglf(const double value);
			bool				SetConstant(const char* name,
											const double value);

			double				Evaluate(const char* exp);
			double				Evaluate(const char* begin, const char* end,
										 const bool allowWhiteSpaces = false);
			const char*			Error();

 private:
			double				_ApplyFunction(const double x,
											   const size_t number) const;

			double				fAglf;
			const char*			fErrorPtr;
			bool				fTrig;

	std::vector<std::string>	fFunctionNames;
	std::vector<char>			fFunctionTypes;

	std::vector<std::string>	fConstantNames;
	std::vector<double>			fConstantValues;
};

#endif // PARSER_H
