/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_REPORT_GENERATOR_H
#define DEBUG_GENERATOR_H

#include <Referenceable.h>


class entry_ref;
class Architecture;
class BString;
class Team;
class Thread;


class DebugReportGenerator : public BReferenceable
{
public:
								DebugReportGenerator(::Team* team,
									Architecture* architecture);
								~DebugReportGenerator();

			void				Init();

	static	DebugReportGenerator* Create(::Team* team,
									Architecture* architecture);

			status_t			GenerateReport(const entry_ref& outputPath);

private:
			void				_Init();

			status_t			_GenerateReportHeader(BString& output);
			status_t			_DumpLoadedImages(BString& output);
			status_t			_DumpRunningThreads(BString& output);
			status_t			_DumpDebuggedThreadInfo(BString& output,
									Thread* thread);

private:
			::Team*				fTeam;
			Architecture*		fArchitecture;
};

#endif // DEBUG_REPORT_GENERATOR_H
