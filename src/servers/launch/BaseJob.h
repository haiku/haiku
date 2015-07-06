/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASE_JOB_H
#define BASE_JOB_H


#include <Job.h>
#include <StringList.h>


using namespace BSupportKit;

class BMessage;
class Condition;
class ConditionContext;


class BaseJob : public BJob {
public:
								BaseJob(const char* name);
								~BaseJob();

			const char*			Name() const;

			const ::Condition*	Condition() const;
			::Condition*		Condition();
			void				SetCondition(::Condition* condition);
			bool				CheckCondition(ConditionContext& context) const;

			const BStringList&	Environment() const;
			const BStringList&	EnvironmentSourceFiles() const;
			void				SetEnvironment(const BMessage& message);

protected:
			::Condition*		fCondition;
			BStringList			fEnvironment;
			BStringList			fSourceFiles;
};


#endif // BASE_JOB_H
