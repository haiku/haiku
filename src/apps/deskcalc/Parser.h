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
  static double valtod(const char *exp, const char **end);
  static double exptod(const char *exp, const char **end);
  static std::string dtostr(const double value, const unsigned int precision, const char mode); // 0 => exp, 1 => exp3, 2 => SI
  static int strprintf(std::string &str, const char * format, ...);

  Expression();

  void SetAglf(const double value);
  bool SetConstant(const char *name, const double value);

  double Eval(const char *exp);
  double Eval(const char *begin, const char *end, const bool allowWhiteSpaces = false);
  const char *Error();

 private:
  double m_aglf;
  const char *m_errorPtr;
  bool m_trig;

  double func(const double x, const size_t number) const;
  std::vector<std::string> m_functionNames;
  std::vector<char> m_functionTypes;

  std::vector<std::string> m_constantNames;
  std::vector<double> m_constantValues;
};

#endif // PARSER_H
