/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H


enum {
	MSG_MODEL_LOADED_SUCCESSFULLY	= 'mlsc',
	MSG_MODEL_LOADED_FAILED			= 'mlfl',
	MSG_MODEL_LOADED_ABORTED		= 'mlab',

	MSG_WINDOW_QUIT					= 'wiqt',

	MSG_CHECK_BOX_RUN_TIME			= 'chkr',
	MSG_CHECK_BOX_WAIT_TIME			= 'chkw',
	MSG_CHECK_BOX_PREEMPTION_TIME	= 'chkp',
	MSG_CHECK_BOX_LATENCY_TIME		= 'chkl'
};


#endif	// MESSAGE_CODES_H
