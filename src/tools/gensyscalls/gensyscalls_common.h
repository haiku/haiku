// gensyscalls_common.h

#ifndef _GEN_SYSCALLS_COMMON_H
#define _GEN_SYSCALLS_COMMON_H

#include <string>

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

	virtual ~Exception() {}

	virtual const char *what() const
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
	virtual ~EOFException() {}
};

// IOException
struct IOException : public Exception {
	IOException() {}
	IOException(const string &message) : Exception(message) {}
	virtual ~IOException() {}
};

// ParseException
struct ParseException : public Exception {
	ParseException() {}
	ParseException(const string &message) : Exception(message) {}
	virtual ~ParseException() {}
};




#endif	// _GEN_SYSCALLS_COMMON_H
