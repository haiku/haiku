#ifndef GLDISPATCHER_H
#define GLDISPATCHER_H


#include <BeBuild.h>
#include <SupportDefs.h>

extern "C" {
#include "glapi.h"
}

class BGLDispatcher
{
		// Private unimplemented copy constructors
		BGLDispatcher(const BGLDispatcher &);
		BGLDispatcher & operator=(const BGLDispatcher &);
	
	public:
		BGLDispatcher();
		~BGLDispatcher();
		
		const char *			Version();
		
		void 					SetCurrentContext(void * context);
		void *					CurrentContext();
		
		struct _glapi_table * 	Table();
		status_t				CheckTable(const struct _glapi_table *dispatch = NULL);
		status_t				SetTable(struct _glapi_table *dispatch);
		uint32					TableSize();
		
		const _glapi_proc 		operator[](const char *function_name);
		const char *			operator[](uint32 offset);
	
		uint32					OffsetOf(const char *function_name);
};

// Inlines methods
inline const char * BGLDispatcher::Version()
{
	return _glapi_get_version();
}


inline void BGLDispatcher::SetCurrentContext(void * context)
{
	_glapi_set_context(context);
}


inline void * BGLDispatcher::CurrentContext()
{
	return _glapi_get_context();
}


inline struct _glapi_table * BGLDispatcher::Table()
{
	return _glapi_get_dispatch();
}


inline uint32 BGLDispatcher::TableSize()
{
	return _glapi_get_dispatch_table_size();
}


inline const _glapi_proc BGLDispatcher::operator[](const char *function_name)
{
	return _glapi_get_proc_address(function_name);
}


inline const char * BGLDispatcher::operator[](uint32 offset)
{
	return _glapi_get_proc_name((GLuint) offset);
}


inline uint32 BGLDispatcher::OffsetOf(const char *function_name)
{
	return (uint32) _glapi_get_proc_offset(function_name);
}


#endif	// GLDISPATCHER_H





