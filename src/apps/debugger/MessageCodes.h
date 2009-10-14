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
	MSG_SET_BREAKPOINT					= 'sbrk',
	MSG_CLEAR_BREAKPOINT				= 'cbrk',
	MSG_ENABLE_BREAKPOINT				= 'ebrk',
	MSG_DISABLE_BREAKPOINT				= 'dbrk',

	MSG_THREAD_STATE_CHANGED			= 'tsch',
	MSG_THREAD_CPU_STATE_CHANGED		= 'tcsc',
	MSG_THREAD_STACK_TRACE_CHANGED		= 'tstc',
	MSG_STACK_FRAME_VALUE_RETRIEVED		= 'sfvr',
	MSG_IMAGE_DEBUG_INFO_CHANGED		= 'idic',
	MSG_IMAGE_FILE_CHANGED				= 'ifch',
	MSG_FUNCTION_SOURCE_CODE_CHANGED	= 'fnsc',
	MSG_USER_BREAKPOINT_CHANGED			= 'ubrc',
	MSG_DEBUGGER_EVENT					= 'dbge',
	MSG_LOAD_SETTINGS					= 'ldst',

	MSG_TEXTVIEW_AUTOSCROLL				= 'tvas',

	MSG_TEAM_DEBUGGER_QUIT				= 'dbqt'
};


#endif	// MESSAGE_CODES_H
