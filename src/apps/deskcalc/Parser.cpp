/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 2004 Daniel Wallner. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Daniel Wallner <daniel.wallner@bredband.net>
 */

#include "Parser.h"

#include <iostream.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#define NOLLF 5e-16

using namespace std;

//LineParser::LineParser(istream *stream):
//	m_stream(stream),
//	m_currentLine(0)
//{
//}
//
//void LineParser::AddSeparator(const char c)
//{
//	m_separators.push_back(c);
//}
//
//size_t LineParser::SetToNext(bool toUpperCase)
//{
//	m_line.erase(m_line.begin(), m_line.end());
//	m_unparsed.assign("");
//
//	// Skip newlines
//	while (m_stream->peek() == 0xA || m_stream->peek() == 0xD)
//		m_stream->get();
//
//	while (!m_stream->eof() && !m_stream->fail() && m_stream->peek() != 0xA && m_stream->peek() != 0xD)
//		{
//			string param;
//
//			// Skip whitespaces
//			while (m_stream->peek() == ' ' || m_stream->peek() == '\t')
//	m_unparsed.append(1, char(m_stream->get()));
////	m_separators.push_back(c); add check for separators !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//			if (m_stream->peek() == '"')
//	{
//		// Skip first "
//		m_unparsed.append(1, char(m_stream->get()));
//
//		int c;
//		// Read until "
//		while (!m_stream->fail() && m_stream->peek() != 0xA && m_stream->peek() != 0xD &&
//		 (c = m_stream->get()) != '"' && !m_stream->eof())
//			{
//				m_unparsed.append(1, char(c));
//				if (toUpperCase)
//		param.append(1, char(toupper(c)));
//				else
//		param.append(1, char(c));
//			}
//		if (c == '"')
//			m_unparsed.append(1, char(c));
//	}
//			else
//	{
//		int c;
//		// Read until whitespace
////	m_separators.push_back(c); add check for separators !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//		while (!m_stream->fail() && m_stream->peek() != 0xA && m_stream->peek() != 0xD &&
//		 (c = m_stream->get()) != ' ' && c != '\t' && !m_stream->eof())
//			{
//				m_unparsed.append(1, char(c));
//				if (toUpperCase)
//		param.append(1, char(toupper(c)));
//				else
//		param.append(1, char(c));
//			}
//		if (c == ' ')
//			m_unparsed.append(1, char(c));
//	}
//
//			// Store parameter
//			if (param.size())
//	m_line.push_back(param);
//		}
//
//	// Skip newlines
//	while (m_stream->peek() == 0xA || m_stream->peek() == 0xD)
//		m_stream->get();
//
//	return m_line.size();
//}
//
//void LineParser::WriteCompressed(string *s) const
//{
//	size_t i;
//	for (i = 0; i < m_line.size(); i++)
//		{
//			if (i != 0)
//	{
//		*s += " ";
//	}
//			*s += m_line[i];
//		}
//}
//
//void LineParser::WriteCompressed(ostream *stream) const
//{
//	size_t i;
//	for (i = 0; i < m_line.size(); i++)
//		{
//			if (i != 0)
//	{
//		*stream << " ";
//	}
//			*stream << m_line[i];
//		}
//	*stream << endl;
//}
//
//size_t LineParser::ParsedLines() const
//{
//	return m_currentLine;
//}
//
//const char *LineParser::Param(const size_t param) const
//{
//	return m_line[param].c_str();
//}
//
//const char *LineParser::Line() const
//{
//	return m_unparsed.c_str();
//}


// NOTE: commented because unused

//template<class T>
//class MemBlock {
// public:
//	MemBlock()
//		: fSize(0),
//		  fBuffer(NULL)
//	{}
//
//	MemBlock(const MemBlock<T>& src)
//		: fSize(0),
//		  fBuffer(NULL)
//	{
//		if (src.fBuffer) {
//			SetSize(src.fSize);
//			memcpy(fBuffer, src.fBuffer, src.fSize);
//		}
//	}
//
//	MemBlock<T>& operator=(const MemBlock<T>& src)
//	{
//		if (this != &src) {
//			SetSize(src.fSize);
//			if (fBuffer)
//				memcpy(fBuffer, src.fBuffer, src.fSize);
//		}
//		return (*this);
//	}
//
//	~MemBlock()
//	{
//		free(fBuffer);
//	}
//
//	void SetSize(const size_t size)
//	{
//		size_t oldsize = Size();
//		if (oldsize != size) {
//			if (!fBuffer && size) {
//				fBuffer = malloc(size);
//				if (!fBuffer)
//					throw "Out of memory";
//			} else if (fBuffer && size) {
//				void *newBuf = realloc(fBuffer, size);
//				if (newBuf)
//					fBuffer = newBuf;
//				else
//					throw "Out of memory";
//			} else if (fBuffer && !size) {
//				free(fBuffer);
//				fBuffer = 0;
//			}
//			fSize = size;
//		}
//	}
//	void* Buffer() const { return fBuffer; }
//	T* Access() const { return (T*)fBuffer; }
//	T& operator[](size_t i) const
//	{
//		return Access()[i];
//	}
//	size_t Size() const { return fSize; }
//
// protected:
//	size_t	fSize;
//	void*	fBuffer;
//};
//
//
//static int
//strprintf(string &str, const char * format, ...)
//{
//	va_list argptr;
//
//	char strfix[128];
//
//	va_start(argptr, format);
//	int ret = vsnprintf(strfix, 128, format, argptr);
//	va_end(argptr);
//
//	if (ret == -1) {
//		MemBlock<char> strbuf;
//		strbuf.SetSize(128);
//		while (ret == -1) {
//			strbuf.SetSize(strbuf.Size() * 2);
//			va_start(argptr, format);
//			ret = vsnprintf(strbuf.Access(), strbuf.Size(), format, argptr);
//			va_end(argptr);
//		}
//		str.assign(strbuf.Access());
//	} else if (ret >= 128) {
//		MemBlock<char> strbuf;
//		strbuf.SetSize(ret + 1);
//		va_start(argptr, format);
//		ret = vsnprintf(strbuf.Access(), ret + 1, format, argptr);
//		va_end(argptr);
//		str.assign(strbuf.Access());
//	} else {
//		str.assign(strfix);
//	}
//
//	return ret;
//}


static double
valtod(const char* exp, const char** end)
{
	// http://en.wikipedia.org/wiki/SI_prefix

	char *endptr;
	double temp = strtod(exp, &endptr);
	if (endptr == exp || !*endptr) {
		 if (end)
		 	*end = endptr;
		 return temp;
	}

	if (end)
		*end = endptr + 1;

	switch (*endptr) {
	/*	case 'Y': // yotta
			return temp * 1.E24;
		case 'Z': // zetta
			return temp * 1.E21;
		case 'E': // exa, incompatible with exponent!
			return temp * 1.E18;*/
		case 'P': // peta
			return temp * 1.E15;
		case 'T': // tera
			return temp * 1.E12;
		case 'G': // giga
			return temp * 1.E9;
		case 'M': // mega
			return temp * 1.E6;
		case 'k': // kilo
			return temp * 1.E3;
		case 'h': // hecto
			return temp * 1.E2;
		case 'd':
			if (endptr[1] == 'a') {
				if (end)
					*end = endptr + 2;
				return temp * 1.E1; // deca
			} else
				return temp * 1.E-1; // deci
		case 'c': // centi
			return temp * 1.E-2;
		case 'm': // milli
			return temp * 1.E-3;
		case 'u': // micro
			return temp * 1.E-6;
		case 'n': // nano
			return temp * 1.E-9;
		case 'p': // pico
			return temp * 1.E-12;
		case 'f': // femto
			return temp * 1.E-15;
		case 'a': // atto
			return temp * 1.E-18;
		case 'z': // zepto
			return temp * 1.E-21;
		case 'y': // yocto
			return temp * 1.E-24;
	}
	if (end)
		*end = endptr;

	return temp;
}

// NOTE: commented because unused

//static double
//exptod(const char* str, const char** end)
//{
//	Expression exp;
//
//	const char* evalEnd = str + strlen(str);
//	double temp = exp.Evaluate(str, evalEnd);
//
//	const char* err = exp.Error();
//	if (end)
//		*end = err ? err : evalEnd;
//
//	return temp;
//}
//
//
//static string
//dtostr(const double value, const unsigned int precision, const char mode)
//{
//	if (value == 0.0)
//		return "0";
//
//	string str;
//	char prefix[2];
//	prefix[0] = 0;
//	prefix[1] = 0;
//
//	if (mode == 1) {
//		// exp3
//		double exp = floor(log10(fabs(value)) / 3.) * 3;
//		string format;
//		strprintf(format, "%%.%dGE%%ld", precision);
//		strprintf(str, format.c_str(), value / pow(10., exp), exp);
//	} else if (mode == 2) {
//		// prefix
//		double exp = floor(log10(fabs(value)) / 3.) * 3;
//		string format;
//		strprintf(format, "%%.%dG", precision);
//		strprintf(str, format.c_str(), value / pow(10., exp));
//		switch (int(exp)) {
//			case 24: // yotta
//				prefix[0] = 'Y';
//				break;
//			case 21: // zetta
//				prefix[0] = 'Z';
//				break;
//			case 18: // exa
//				prefix[0] = 'E';
//				break;
//			case 15: // peta
//				prefix[0] = 'P';
//				break;
//			case 12: // tera
//				prefix[0] = 'T';
//				break;
//			case 9: // giga
//				prefix[0] = 'G';
//				break;
//			case 6: // mega
//				prefix[0] = 'M';
//				break;
//			case 3: // kilo
//				prefix[0] = 'k';
//				break;
//			case -3: // milli
//				prefix[0] = 'm';
//				break;
//			case -6: // micro
//				prefix[0] = 'u';
//				break;
//			case -9: // nano
//				prefix[0] = 'n';
//				break;
//			case -12: // pico
//				prefix[0] = 'p';
//				break;
//			case -15: // femto
//				prefix[0] = 'f';
//				break;
//			case -18: // atto
//				prefix[0] = 'a';
//				break;
//			case -21: // zepto
//				prefix[0] = 'z';
//				break;
//			case -24: // yocto
//				prefix[0] = 'y';
//				break;
//			default:
//				if (exp) {
//					strprintf(format, "%%.%dGE%%ld", precision);
//					strprintf(str, format.c_str(), value / pow(10., exp), exp);
//				} else {
//					strprintf(format, "%%.%dG", precision);
//					strprintf(str, format.c_str(), value);
//				}
//				break;
//		}
//	} else {
//		// exp
//		string format;
//		strprintf(format, "%%.%dG", precision);
//		strprintf(str, format.c_str(), value);
//	}
//
//	return string(str) + prefix;
//}


// #pragma mark -


Expression::Expression()
	: fAglf(1.),
	  fErrorPtr(NULL),
	  fTrig(false)
{
	// NOTE: these seem to be hard coded anyways...
	fFunctionNames.push_back("sin(");	// 0
	fFunctionTypes.push_back(1);
	fFunctionNames.push_back("asin(");	// 1
	fFunctionTypes.push_back(2);
	fFunctionNames.push_back("cos(");	// 2
	fFunctionTypes.push_back(1);
	fFunctionNames.push_back("acos(");	// 3
	fFunctionTypes.push_back(2);
	fFunctionNames.push_back("tan(");	// 4
	fFunctionTypes.push_back(1);
	fFunctionNames.push_back("atan(");	// 5
	fFunctionTypes.push_back(2);
	fFunctionNames.push_back("ln(");	// 6
	fFunctionTypes.push_back(0);
	fFunctionNames.push_back("lg(");	// 7
	fFunctionTypes.push_back(0);
	fFunctionNames.push_back("sqrt(");	// 8
	fFunctionTypes.push_back(0);
	fFunctionNames.push_back("(");		// 9
	fFunctionTypes.push_back(0);

	SetConstant("e", M_E);
	SetConstant("pi", M_PI);
}


void
Expression::SetAglf(const double value)
{
	fAglf = value;
}


bool
Expression::SetConstant(const char* name, const double value)
{
	for (size_t i = 0; name[i]; i++) {
		if (!isalnum(name[i])) {
			throw "Illegal character in constant name";
		}
	}
	for (size_t i = 0; i < fConstantNames.size(); i++) {
		if (!strcmp(name, fConstantNames[i].c_str())) {
			fConstantValues[i] = value;
			return true;
		}
	}

	fConstantNames.push_back(name);
	fConstantValues.push_back(value);

	return false;
}


double
Expression::Evaluate(const char* exp)
{
	const char* evalEnd = exp + strlen(exp);
	return Evaluate(exp, evalEnd);
}


double
Expression::Evaluate(const char* begin, const char* end, const bool allowWhiteSpaces)
{
	const char* curptr = begin;
	int state = 1;
	double operand;
	double sum = 0.;
	double prod = 0.0;
	double power = 0.0;

	if (begin[0] == 0) {
		fErrorPtr = begin;
		return 0.;
	}

	if (fErrorPtr) {
		return 0.;
	}

	for (;;) {
		const char *operandEnd;
		bool minusBeforeOperand = false;
		bool numberOperand = true;

		if (allowWhiteSpaces) {
			while (curptr < end && isspace(curptr[0]))
				curptr++;
		}

		if (curptr[0] == '-')
			minusBeforeOperand = true;

		operand = valtod(curptr, &operandEnd);

		if (operandEnd == curptr) {
			// operand not a number
			const char *operandStart = curptr;
			double operandSign = 1.;

			numberOperand = false;

			if (curptr[0] == '+') {
				++operandStart;
			} else if (curptr[0] == '-') {
				++operandStart;
				operandSign = -1.;
			}

			// Iterate through functions
			size_t i;
			for (i = 0; i < fFunctionNames.size(); i++) {
				if (!strncmp(operandStart, fFunctionNames[i].c_str(), fFunctionNames[i].size())) {
					const char *functionArg = operandStart + fFunctionNames[i].size();
					const char *chptr = functionArg - 1;
					const char *prptr = functionArg - 1;
		
					if (fFunctionTypes[i])
						fTrig = true;
		
					// Find closing paranthesis
					while (prptr) {
						chptr = strchr(chptr + 1, ')');
						prptr = strchr(prptr + 1, '(');
						if (prptr > chptr)
							break;
					}

					if (chptr) {
						// Closing paranthesis
						if (fFunctionTypes[i] == 1) {
							// Trig
							operand = operandSign * _ApplyFunction(fAglf * Evaluate(functionArg, chptr), i);
							if (fabs(operand) < NOLLF)
								operand = 0;
						} else if (fFunctionTypes[i] == 2) {
							// Inv trig
							operand = operandSign / fAglf * _ApplyFunction(Evaluate(functionArg, chptr), i);
						} else {
							// Normal
							operand = operandSign * _ApplyFunction(Evaluate(functionArg, chptr), i);
						}
						operandEnd = chptr + 1;
					} else {
						// No closing paranthesis
						if (fFunctionTypes[i] == 1) {
							operand = operandSign * _ApplyFunction(fAglf * Evaluate(functionArg, end), i);
							if (fabs(operand) < NOLLF)
								operand = 0;
						} else if (fFunctionTypes[i] == 2) {
							operand = operandSign / fAglf * _ApplyFunction(Evaluate(functionArg, end), i);
						} else {
							operand = operandSign * _ApplyFunction(Evaluate(functionArg, end), i);
						}
						operandEnd = end;
					}

					if (fErrorPtr) {
						return 0.;
					}

					break;
				}
			}

			// Iterate through constants
			if (i == fFunctionNames.size()) {
				// Only search if no function found
				for (i = 0; i < fConstantNames.size(); i++) {
					if (!strncmp(operandStart, fConstantNames[i].c_str(), fConstantNames[i].size()) && !isalnum(operandStart[fConstantNames[i].size()])) {
						operandEnd = operandStart + fConstantNames[i].size();
						operand = operandSign * fConstantValues[i];
						break;
					}
				}
			}
		}

		if (operandEnd > end) {
			fErrorPtr = end;
			return 0.;
		}

		// Calculate expression in correct order

		//	state == 1	=> term				=> sum + operand
		//	state == 2	=> product			=> sum + prod * operand
		//	state == 3	=> quotient			=> sum + prod / operand
		//	state == 4	=> plus & power		=> sum + power ^ operand
		//	state == 5	=> minus & power	=> sum - power ^ operand
		//	state == 6	=> product & power	=> sum + prod * power ^ operand
		//	state == 7	=> quotient & power	=> sum + prod / power ^ operand

		if (curptr != operandEnd) {
			// number?
			switch (state) {
				case 1:	// term
					if (operandEnd == end) {
						return sum + operand;
					}
					switch (operandEnd[0]) {	
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + operand;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = operand;
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = operand;
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = operand;
							if (minusBeforeOperand) {
								power = power * -1.;
								state = 5;
							} else {
								state = 4;
							}
							break;
						default:
							if (numberOperand && isalpha(operandEnd[0])) {
								// Assume multiplication of function/constant without *
								curptr = operandEnd;
								prod = operand;
								state = 2;
							} else {
								fErrorPtr = operandEnd;
								return 0.;
							}
					}
					break;

				case 2: // product
					if (operandEnd == end) {
						return sum + prod * operand;
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + prod * operand;
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = prod * operand;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = prod * operand;
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = operand;
							state = 6;
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;

				case 3: // quotient
					if (operandEnd == end) {
						return sum + prod / operand;
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + prod / operand;
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = prod / operand;
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = prod / operand;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = operand;
							state = 7;
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;

				case 4: // plus&power
					if (operandEnd == end) {
						return sum + pow(power, operand);
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + pow(power, operand);
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = pow(power, operand);
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = pow(power, operand);
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = pow(power, operand);
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;

				case 5: // minus&power
					if (operandEnd == end) {
						return sum - pow(power, operand);
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum - pow(power, operand);
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = pow(power, operand);
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = pow(power, operand);
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = pow(power, operand);
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;

				case 6: // product&power
					if (operandEnd == end) {
						return sum + prod * pow(power, operand);
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + prod * pow(power, operand);
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = prod * pow(power, operand);
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = prod * pow(power, operand);
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = pow(power, operand);
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;
	
				case 7: // quotient&power
					if (operandEnd == end) {
						return sum + prod / pow(power, operand);
					}
					switch (operandEnd[0]) {
						// Check next
						case '+':
						case '-':
							curptr = operandEnd;
							sum = sum + prod / pow(power, operand);
							state = 1;
							break;
						case '*':
							curptr = operandEnd + 1;
							prod = prod / pow(power, operand);
							state = 2;
							break;
						case '/':
							curptr = operandEnd + 1;
							prod = prod / pow(power, operand);
							state = 3;
							break;
						case '^':
							curptr = operandEnd + 1;
							power = pow(power, operand);
							break;
						default:
							fErrorPtr = operandEnd;
							return 0.;
					}
					break;
	
				default:
					fErrorPtr = operandEnd;
					return 0.;
			}
		} else {
			fErrorPtr = curptr;
			return 0.;
		}
	}
}


const char*
Expression::Error()
{
	const char *retval = fErrorPtr;
	fErrorPtr = NULL;
	fTrig = false;
	return retval;
}


// #pragma mark -


double
Expression::_ApplyFunction(const double x, const size_t number) const
{
	switch (number) {
		case 0:
			return sin(x);
		case 1:
			return asin(x);
		case 2:
			return cos(x);
		case 3:
			return acos(x);
		case 4:
			return tan(x);
		case 5:
			return atan(x);
		case 6:
			return log(x);
		case 7:
			return log10(x);
		case 8:
			return sqrt(x);
		default:
			return x;
	}
}
