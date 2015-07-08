/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONDITIONS_H
#define CONDITIONS_H


#include <String.h>


class BMessage;


class ConditionContext {
public:
	virtual	bool				IsSafeMode() const = 0;
	virtual	bool				BootVolumeIsReadOnly() const = 0;
};


class Condition {
public:
								Condition();
	virtual						~Condition();

	virtual	bool				Test(ConditionContext& context) const = 0;

	/*!	Determines whether or not the result of this condition is fixed,
		and will not change anymore.
	*/
	virtual	bool				IsConstant(ConditionContext& context) const;

	virtual	BString				ToString() const = 0;
};


class Conditions {
public:
	static	Condition*			FromMessage(const BMessage& message);
	static	Condition*			AddNotSafeMode(Condition* condition);
};


#endif // CONDITIONS_H
