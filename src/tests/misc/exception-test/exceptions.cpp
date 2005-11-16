
#include "exceptions.h"

ExceptionBase::ExceptionBase()	{}
ExceptionBase::~ExceptionBase()	{}

ExceptionA::ExceptionA()	{}
ExceptionA::~ExceptionA()	{}
	
ExceptionB::ExceptionB()	{}
ExceptionB::~ExceptionB()	{}
	
VirtualExceptionBase::VirtualExceptionBase()	{}
VirtualExceptionBase::~VirtualExceptionBase()	{}
	
VirtualExceptionA::VirtualExceptionA()	{}
VirtualExceptionA::~VirtualExceptionA()	{}

VirtualExceptionB::VirtualExceptionB()	{}
VirtualExceptionB::~VirtualExceptionB()	{}

void throwBase()		{ throw ExceptionBase(); }
void throwA()			{ throw ExceptionA(); }
void throwB()			{ throw ExceptionB(); }
void throwVirtualBase()	{ throw VirtualExceptionBase(); }
void throwVirtualA()	{ throw VirtualExceptionA(); }
void throwVirtualB()	{ throw VirtualExceptionB(); }
void throwInt()			{ throw int(7); }
