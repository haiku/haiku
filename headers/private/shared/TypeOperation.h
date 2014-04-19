/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on TypeOp in "C++-Templates, The Complete Guide", which is
 * Copyright 2003 by Pearson Education, Inc.
 */
#ifndef _TYPE_OPERATION_H
#define _TYPE_OPERATION_H

namespace BPrivate {


// Generic type conversion operations.
template<typename T>
class TypeOperation {
public:
	typedef T					ArgT;
	typedef T					BareT;
	typedef T const				ConstT;

	typedef T&					RefT;
	typedef T&					BareRefT;
	typedef T const&			ConstRefT;
};


// Specialization for constant types.
template<typename T>
class TypeOperation<T const> {
public:
	typedef T const				ArgT;
	typedef T					BareT;
	typedef T const				ConstT;

	typedef T const&			RefT;
	typedef T&					BareRefT;
	typedef T const&			ConstRefT;
};


// Specialization for reference types.
template<typename T>
class TypeOperation<T&> {
public:
	typedef T&									ArgT;
	typedef typename TypeOperation<T>::BareT	BareT;
	typedef T const								ConstT;

	typedef T&									RefT;
	typedef typename TypeOperation<T>::BareRefT	BareRefT;
	typedef T const&							ConstRefT;
};


// Specialization for void.
template<>
class TypeOperation<void> {
public:
	typedef void				ArgT;
	typedef void				BareT;
	typedef void const			ConstT;

	typedef void				RefT;
	typedef void				BareRefT;
	typedef void				ConstRefT;
};


}	// namespace BPrivate


using BPrivate::TypeOperation;


#endif	// _TYPE_OPERATION_H
