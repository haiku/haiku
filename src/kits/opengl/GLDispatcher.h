#ifndef GLDISPATCHER_H
#define GLDISPATCHER_H


#include <BeBuild.h>
#include <SupportDefs.h>

extern "C" {
#include "glapi.h"
}

class GLDispatcher
{
		// Private unimplemented copy constructors
		GLDispatcher(const GLDispatcher &);
		GLDispatcher & operator=(const GLDispatcher &);
	
	public:
		GLDispatcher();
		~GLDispatcher();
		
		const char *			Version();
		
		void 					SetCurrentContext(void * context);
		void *					CurrentContext();
		
		struct _glapi_table * 	Table();
		status_t				CheckTable(const struct _glapi_table *dispatch = NULL);
		status_t				SetTable(struct _glapi_table *dispatch);
		uint32					TableSize();
		
		const _glapi_proc 		operator[](const char *function_name);
	
		const char *			FunctionName(uint32 offset);
		uint32					FunctionOffset(const char *function_name);
};

// Inlines methods
inline const char * GLDispatcher::Version()
{
	return _glapi_get_version();
}


inline void GLDispatcher::SetCurrentContext(void * context)
{
	_glapi_set_context(context);
}


inline void * GLDispatcher::CurrentContext()
{
	return _glapi_get_context();
}


inline struct _glapi_table * GLDispatcher::Table()
{
	return _glapi_get_dispatch();
}


inline uint32 GLDispatcher::TableSize()
{
	return _glapi_get_dispatch_table_size();
}


inline const _glapi_proc GLDispatcher::operator[](const char *function_name)
{
	return _glapi_get_proc_address(function_name);
}


inline const char * GLDispatcher::FunctionName(uint32 offset)
{
	return _glapi_get_proc_name((GLuint) offset);
}


inline uint32 GLDispatcher::FunctionOffset(const char *function_name)
{
	return (uint32) _glapi_get_proc_offset;
}


#endif	// GLDISPATCHER_H





