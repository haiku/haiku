/*
 * Copyright 2000-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Paul <brian.e.paul@gmail.com>
 */
#ifndef GLDISPATCHER_H
#define GLDISPATCHER_H


#include <BeBuild.h>
#include <GL/gl.h>
#include <SupportDefs.h>

#include "glheader.h"

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

		void 					SetCurrentContext(void * context);
		void *					CurrentContext();

		struct _glapi_table * 	Table();
		status_t				CheckTable(
									const struct _glapi_table *dispatch = NULL);
		status_t				SetTable(struct _glapi_table *dispatch);
		uint32					TableSize();

		const _glapi_proc 		operator[](const char *functionName);
		const char *			operator[](uint32 offset);

		const _glapi_proc		AddressOf(const char *functionName);
		uint32					OffsetOf(const char *functionName);
};


// Inlines methods
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


inline const _glapi_proc BGLDispatcher::operator[](const char *functionName)
{
	return _glapi_get_proc_address(functionName);
}


inline const char * BGLDispatcher::operator[](uint32 offset)
{
	return _glapi_get_proc_name((GLuint) offset);
}


inline const _glapi_proc BGLDispatcher::AddressOf(const char *functionName)
{
	return _glapi_get_proc_address(functionName);
}


inline uint32 BGLDispatcher::OffsetOf(const char *functionName)
{
	return (uint32) _glapi_get_proc_offset(functionName);
}


#endif	// GLDISPATCHER_H
