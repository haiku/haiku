//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//		  mcseemagg@yahoo.com
//		  http://www.antigrain.com
//----------------------------------------------------------------------------
//
// SVG exception
//
//----------------------------------------------------------------------------

#ifndef SVG_EXCEPTION_H
#define SVG_EXCEPTION_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace agg  { 
namespace svg {

class exception {
 public:

	~exception()
	{
		delete [] m_msg;
	}

	exception() : m_msg(0) {}
	  
	exception(const char* fmt, ...) :
		m_msg(0)
	{
		if(fmt) 
		{
			m_msg = new char [4096];
			va_list arg;
			va_start(arg, fmt);
			vsprintf(m_msg, fmt, arg);
			va_end(arg);
		}
	}

	exception(const exception& exc) :
		m_msg(exc.m_msg ? new char[strlen(exc.m_msg) + 1] : 0)
	{
		if(m_msg) strcpy(m_msg, exc.m_msg);
	}
	
	const char* msg() const { return m_msg; }

 private:
	char* m_msg;
};

} // namespace svg
} // namespace agg

#endif // SVG_EXCEPTION_H
