/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNAL_INFO_H
#define SIGNAL_INFO_H

#include <signal.h>

#include "Types.h"


class SignalInfo {
public:
								SignalInfo();
								SignalInfo(const SignalInfo& other);
								SignalInfo(int signal,
									const struct sigaction& handler,
									bool deadly);

			void				SetTo(int signal,
									const struct sigaction& handler,
									bool deadly);

			int					Signal() const	{ return fSignal; }
			const struct sigaction&	Handler() const	{ return fHandler; }
			bool				Deadly() const 	{ return fDeadly; }
private:
			int 				fSignal;
			struct sigaction	fHandler;
			bool				fDeadly;
};


#endif	// SIGNAL_INFO_H
