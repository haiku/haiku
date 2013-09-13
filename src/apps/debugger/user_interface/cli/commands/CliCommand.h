/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_COMMAND_H
#define CLI_COMMAND_H


#include <Referenceable.h>


class CliContext;


class CliCommand : public BReferenceable {
public:
								CliCommand(const char* summary,
									const char* usage);
	virtual						~CliCommand();

			const char*			Summary() const	{ return fSummary; }
			const char*			Usage() const	{ return fUsage; }

			void				PrintUsage(const char* commandName) const;

	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context) = 0;

private:
			const char*			fSummary;
			const char*			fUsage;
};


#endif	// CLI_COMMAND_H
