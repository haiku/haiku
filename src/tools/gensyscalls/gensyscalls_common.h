// gensyscalls_common.h

#ifndef _GEN_SYSCALLS_COMMON_H
#define _GEN_SYSCALLS_COMMON_H

#include <exception>
#include <string>

using namespace std;

// Exception
struct Exception : exception {
	Exception()
		: fMessage()
	{
	}

	Exception(const string &message)
		: fMessage(message)
	{
	}

	virtual ~Exception() throw() {}

	virtual const char *what() const throw()
	{
		return fMessage.c_str();
	}

private:
	string	fMessage;
};

// EOFException
struct EOFException : public Exception {
	EOFException() {}
	EOFException(const string &message) : Exception(message) {}
	virtual ~EOFException() throw() {}
};

// IOException
struct IOException : public Exception {
	IOException() {}
	IOException(const string &message) : Exception(message) {}
	virtual ~IOException() throw() {}
};

// ParseException
struct ParseException : public Exception {
	ParseException() {}
	ParseException(const string &message) : Exception(message) {}
	virtual ~ParseException() throw() {}
};

#endif	// _GEN_SYSCALLS_COMMON_H
