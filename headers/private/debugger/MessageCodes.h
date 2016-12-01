/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H


enum {
	MSG_THREAD_RUN								= 'run_',
	MSG_THREAD_SET_ADDRESS						= 'sead',
	MSG_THREAD_STOP								= 'stop',
	MSG_THREAD_STEP_OVER						= 'stov',
	MSG_THREAD_STEP_INTO						= 'stin',
	MSG_THREAD_STEP_OUT							= 'stou',
	MSG_SET_BREAKPOINT							= 'sbrk',
	MSG_CLEAR_BREAKPOINT						= 'cbrk',
	MSG_ENABLE_BREAKPOINT						= 'ebrk',
	MSG_DISABLE_BREAKPOINT						= 'dbrk',
	MSG_SET_BREAKPOINT_CONDITION				= 'sbpc',
	MSG_CLEAR_BREAKPOINT_CONDITION				= 'cbpc',
	MSG_SET_WATCHPOINT							= 'swpt',
	MSG_CLEAR_WATCHPOINT						= 'cwpt',
	MSG_ENABLE_WATCHPOINT						= 'ewpt',
	MSG_DISABLE_WATCHPOINT						= 'dwpt',
	MSG_STOP_ON_IMAGE_LOAD						= 'tsil',
	MSG_ADD_STOP_IMAGE_NAME						= 'asin',
	MSG_REMOVE_STOP_IMAGE_NAME					= 'rsin',
	MSG_SET_DEFAULT_SIGNAL_DISPOSITION			= 'sdsd',
	MSG_SET_CUSTOM_SIGNAL_DISPOSITION			= 'scsd',
	MSG_REMOVE_CUSTOM_SIGNAL_DISPOSITION		= 'rcsd',

	MSG_TEAM_RENAMED							= 'tera',
	MSG_THREAD_STATE_CHANGED					= 'tsch',
	MSG_THREAD_CPU_STATE_CHANGED				= 'tcsc',
	MSG_THREAD_STACK_TRACE_CHANGED				= 'tstc',
	MSG_IMAGE_DEBUG_INFO_CHANGED				= 'idic',
	MSG_CONSOLE_OUTPUT_RECEIVED					= 'core',
	MSG_IMAGE_FILE_CHANGED						= 'ifch',
	MSG_FUNCTION_SOURCE_CODE_CHANGED			= 'fnsc',
	MSG_USER_BREAKPOINT_CHANGED					= 'ubrc',
	MSG_WATCHPOINT_CHANGED						= 'wapc',
	MSG_MEMORY_DATA_CHANGED						= 'mdac',
	MSG_DEBUGGER_EVENT							= 'dbge',
	MSG_LOAD_SETTINGS							= 'ldst',

	MSG_VALUE_NODE_CHANGED						= 'vnch',
	MSG_VALUE_NODE_CHILDREN_CREATED				= 'vncc',
	MSG_VALUE_NODE_CHILDREN_DELETED				= 'vncd',
	MSG_VALUE_NODE_VALUE_CHANGED				= 'vnvc',

	MSG_INSPECT_ADDRESS							= 'isad',
	MSG_WRITE_TARGET_MEMORY						= 'wtam',
	MSG_EVALUATE_EXPRESSION						= 'evex',
	MSG_WRITE_CORE_FILE							= 'wrcf',

	MSG_TEAM_DEBUGGER_QUIT						= 'dbqt',
	MSG_TEAM_RESTART_REQUESTED					= 'trrq',

	MSG_GENERATE_DEBUG_REPORT					= 'gdrp',
	MSG_DEBUG_INFO_NEEDS_USER_INPUT				= 'dnui',
	MSG_USER_INTERFACE_FILE_CHOSEN				= 'uifc'
};


#endif	// MESSAGE_CODES_H
