/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNAL_DISPOSITION_TYPES_H
#define SIGNAL_DISPOSITION_TYPES_H


enum SignalDisposition {
	SIGNAL_DISPOSITION_IGNORE = 0,
	SIGNAL_DISPOSITION_STOP_AT_RECEIPT,
	SIGNAL_DISPOSITION_STOP_AT_SIGNAL_HANDLER,
		// NB: if the team has no signal handler installed for
		// the corresponding signal, this implies stop at receipt.

	SIGNAL_DISPOSITION_MAX
};


#endif	// SIGNAL_DISPOSITION_TYPES_H
