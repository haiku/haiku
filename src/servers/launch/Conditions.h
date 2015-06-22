/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONDITIONS_H
#define CONDITIONS_H


class BMessage;


class ConditionContext {
public:
	virtual	bool				IsSafeMode() const = 0;
};


class Condition {
public:
								Condition();
	virtual						~Condition();

	virtual	bool				Test(ConditionContext& context) const = 0;
};


class Conditions {
public:
	static	Condition*			FromMessage(const BMessage& message);
};


#endif // CONDITIONS_H
