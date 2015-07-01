/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASE_JOB_H
#define BASE_JOB_H


#include <Job.h>


using namespace BSupportKit;

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

protected:
			::Condition*		fCondition;
};


#endif // BASE_JOB_H
