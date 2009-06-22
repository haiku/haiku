/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H


enum {
	MSG_THREAD_RUN						= 'run_',
	MSG_THREAD_STOP						= 'stop',
	MSG_THREAD_STEP_OVER				= 'stov',
	MSG_THREAD_STEP_INTO				= 'stin',
	MSG_THREAD_STEP_OUT					= 'stou',
	MSG_SET_BREAKPONT					= 'sbrk',
	MSG_CLEAR_BREAKPONT					= 'cbrk',

	MSG_THREAD_STATE_CHANGED			= 'tsch',
	MSG_THREAD_CPU_STATE_CHANGED		= 'tcsc',
	MSG_THREAD_STACK_TRACE_CHANGED		= 'tstc',
	MSG_STACK_FRAME_SOURCE_CODE_CHANGED	= 'sfsc',
	MSG_USER_BREAKPOINT_CHANGED			= 'ubrc'
};


#endif	// MESSAGE_CODES_H
