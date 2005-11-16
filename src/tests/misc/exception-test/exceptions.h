
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

struct ExceptionBase {
	ExceptionBase();
	~ExceptionBase();
};

struct ExceptionA : ExceptionBase {
	ExceptionA();
	~ExceptionA();

	int a;
};

struct ExceptionB : ExceptionBase {
	ExceptionB();
	~ExceptionB();

	int b;
};

struct VirtualExceptionBase : ExceptionBase {
	VirtualExceptionBase();
	virtual ~VirtualExceptionBase();
};

struct VirtualExceptionA : VirtualExceptionBase {
	VirtualExceptionA();
	virtual ~VirtualExceptionA();

	int a;
};

struct VirtualExceptionB : VirtualExceptionBase {
	VirtualExceptionB();
	virtual ~VirtualExceptionB();

	int b;
};

void throwBase();
void throwA();
void throwB();
void throwVirtualBase();
void throwVirtualA();
void throwVirtualB();
void throwInt();


#endif	// EXCEPTIONS_H
