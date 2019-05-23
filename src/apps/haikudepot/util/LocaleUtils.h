/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_UTILS_H
#define LOCALE_UTILS_H


class BCollator;


class LocaleUtils {

public:
	static	BCollator*		GetSharedCollator();

private:
	static	void			GetCollator(BCollator* collator);

private:
	static	BCollator*		sSharedCollator;
};


#endif // LOCALE_UTILS_H
