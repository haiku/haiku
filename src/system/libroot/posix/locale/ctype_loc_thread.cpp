/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>

#include "LocaleInternal.h"


using BPrivate::Libroot::GetCurrentThreadLocale;


extern "C" const unsigned short int *const *const
__ctype_b_loc()
{
	return &GetCurrentThreadLocale()->ctype_b;
}


extern "C" const int *const *const
__ctype_tolower_loc()
{
	return &GetCurrentThreadLocale()->ctype_tolower;
}


extern "C" const int *const *const
__ctype_toupper_loc()
{
	return &GetCurrentThreadLocale()->ctype_toupper;
}


extern "C" unsigned short
__ctype_get_mb_cur_max()
{
	return *GetCurrentThreadLocale()->mb_cur_max;
}
