/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H


enum {
	MSG_THREAD_RUN								= 'run_',
	MSG_THREAD_STOP								= 'stop',
	MSG_THREAD_STEP_OVER						= 'stov',
	MSG_THREAD_STEP_INTO						= 'stin',
	MSG_THREAD_STEP_OUT							= 'stou',
	MSG_SET_BREAKPOINT							= 'sbrk',
	MSG_CLEAR_BREAKPOINT						= 'cbrk',
	MSG_ENABLE_BREAKPOINT						= 'ebrk',
	MSG_DISABLE_BREAKPOINT						= 'dbrk',

	MSG_THREAD_STATE_CHANGED					= 'tsch',
	MSG_THREAD_CPU_STATE_CHANGED				= 'tcsc',
	MSG_THREAD_STACK_TRACE_CHANGED				= 'tstc',
	MSG_STACK_FRAME_VALUE_RETRIEVED				= 'sfvr',
	MSG_IMAGE_DEBUG_INFO_CHANGED				= 'idic',
	MSG_IMAGE_FILE_CHANGED						= 'ifch',
	MSG_FUNCTION_SOURCE_CODE_CHANGED			= 'fnsc',
	MSG_USER_BREAKPOINT_CHANGED					= 'ubrc',
	MSG_DEBUGGER_EVENT							= 'dbge',
	MSG_LOAD_SETTINGS							= 'ldst',

	MSG_SETTINGS_MENU_IMPL_ITEM_SELECTED		= 'smii',
	MSG_SETTINGS_MENU_IMPL_OPTION_ITEM_SELECTED	= 'smio',

	MSG_TEXTVIEW_AUTOSCROLL						= 'tvas',

	MSG_VARIABLES_VIEW_CONTEXT_MENU_DONE		= 'ctxd',
	MSG_VARIABLES_VIEW_NODE_SETTINGS_CHANGED	= 'vvns',

	MSG_VALUE_NODE_CHANGED						= 'vnch',
	MSG_VALUE_NODE_CHILDREN_CREATED				= 'vncc',
	MSG_VALUE_NODE_CHILDREN_DELETED				= 'vncd',
	MSG_VALUE_NODE_VALUE_CHANGED				= 'vnvc',

	MSG_TEAM_DEBUGGER_QUIT						= 'dbqt',
	MSG_SHOW_TEAMS_WINDOW						= 'stsw',
	MSG_TEAMS_WINDOW_CLOSED						= 'tswc',
	MSG_DEBUG_THIS_TEAM							= 'dbtt',
	MSG_SHOW_INSPECTOR_WINDOW					= 'sirw',
	MSG_INSPECTOR_WINDOW_CLOSED					= 'irwc',
	MSG_INSPECT_ADDRESS							= 'isad'
};


#endif	// MESSAGE_CODES_H
