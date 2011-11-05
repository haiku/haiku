/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// functional_tools.h
//
// PURPOSE
//   Some extra STL-related functors, adaptors, & helper functions.
//
// NOTES
//   bound_method() makes it easy to use STL algorithms to apply a given
//     method in a given object to a set of instances of another class.
//
//   The additional-argument functors aren't perfected; most notably,
//     bind2nd() doesn't handle bound methods that take objects by
//     reference.  +++++
//
// HISTORY
//   e.moon 27jul99		begun

#ifndef __functional_tools_H__
#define __functional_tools_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

///////////////////////////////////////////////////////////////////////////
// bound_method
///////////////////////////////////////////////////////////////////////////

// ***** no additional arguments *****

// 27jul99: functor adaptor "call a given method of a given object with argument"
template<class _retT, class _subjectT, class _objectT>
class bound_method_t : public std::unary_function<_objectT*, _retT> {
public:
	explicit bound_method_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(_objectT*)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(_objectT* o) const {
		return (_m_subject.*_m_method)(o);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(_objectT*);
};

// 27jul99: functor adaptor "call a given method of a given object with argument"
template<class _retT, class _subjectT, class _objectT>
class bound_const_method_t : public std::unary_function<const _objectT*, _retT> {
public:
	explicit bound_const_method_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(const _objectT*)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(const _objectT* o) const {
		return (_m_subject.*_m_method)(o);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(const _objectT*);
};

template<class _retT, class _subjectT, class _objectT>
class bound_method_ref_t : public std::unary_function<_objectT&, _retT> {
public:
	explicit bound_method_ref_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(_objectT&)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(_objectT& o) const {
		return (_m_subject.*_m_method)(o);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(_objectT&);
};

template<class _retT, class _subjectT, class _objectT>
class bound_const_method_ref_t : public std::unary_function<const _objectT&, _retT> {
public:
	explicit bound_const_method_ref_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(const _objectT&)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(const _objectT& o) const {
		return (_m_subject.*_m_method)(o);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(const _objectT&);
};

// ***** 1 additional argument *****

// 27jul99: functor adaptor "call a given method of a given object with argument"
//          + an additional argument
template<class _retT, class _subjectT, class _objectT, class _arg1T>
class bound_method1_t : public std::binary_function<_objectT*,_arg1T,_retT> {
public:
	explicit bound_method1_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(_objectT*, _arg1T)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(_objectT* o, _arg1T arg1) const {
		return (_m_subject.*_m_method)(o, arg1);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(_objectT*, _arg1T);
};


// 27jul99: functor adaptor "call a given method of a given object with argument"
template<class _retT, class _subjectT, class _objectT, class _arg1T>
class bound_const_method1_t : public std::binary_function<const _objectT*,_arg1T,_retT> {
public:
	explicit bound_const_method1_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(const _objectT*, _arg1T)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(const _objectT* o, _arg1T arg1) const{
		return (_m_subject.*_m_method)(o, arg1);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(const _objectT*,_arg1T);
};

template<class _retT, class _subjectT, class _objectT, class _arg1T>
class bound_method_ref1_t : public std::binary_function<_objectT&,_arg1T,_retT> {
public:
	explicit bound_method_ref1_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(_objectT&,_arg1T)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(_objectT& o, _arg1T arg1) const {
		return (_m_subject.*_m_method)(o, arg1);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(_objectT&,_arg1T);
};

template<class _retT, class _subjectT, class _objectT, class _arg1T>
class bound_const_method_ref1_t : public std::binary_function<const _objectT&,_arg1T,_retT> {
public:
	explicit bound_const_method_ref1_t(
		// the bound instance on which the method will be called
		_subjectT& __subject,
		// the method
		_retT (_subjectT::* __method)(const _objectT&,_arg1T)) : _m_subject(__subject), _m_method(__method) {}
		
	_retT operator()(const _objectT& o, _arg1T arg1) const {
		return (_m_subject.*_m_method)(o, arg1);
	}
		
private:
	_subjectT&						_m_subject;
	_retT (_subjectT::*		_m_method)(const _objectT&,_arg1T);
};


// 27jul99: adaptor functions

// ***** 0-argument *****

template<class _retT, class _subjectT, class _objectT>
inline bound_method_t<_retT,_subjectT,_objectT> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(_objectT*)) {
	return bound_method_t<_retT,_subjectT,_objectT>(subject, method);
}

template<class _retT, class _subjectT, class _objectT>
inline bound_const_method_t<_retT,_subjectT,_objectT> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(const _objectT*)) {
	return bound_const_method_t<_retT,_subjectT,_objectT>(subject, method);
}

template<class _retT, class _subjectT, class _objectT>
inline bound_method_ref_t<_retT,_subjectT,_objectT> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(_objectT&)) {
	return bound_method_ref_t<_retT,_subjectT,_objectT>(subject, method);
}

template<class _retT, class _subjectT, class _objectT>
inline bound_const_method_ref_t<_retT,_subjectT,_objectT> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(const _objectT&)) {
	return bound_const_method_ref_t<_retT,_subjectT,_objectT>(subject, method);
}

// ***** 1-argument *****

template<class _retT, class _subjectT, class _objectT, class _arg1T>
inline bound_method1_t<_retT,_subjectT,_objectT,_arg1T> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(_objectT*,_arg1T)) {
	return bound_method1_t<_retT,_subjectT,_objectT,_arg1T>(subject, method);
}

template<class _retT, class _subjectT, class _objectT, class _arg1T>
inline bound_const_method1_t<_retT,_subjectT,_objectT,_arg1T> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(const _objectT*,_arg1T)) {
	return bound_const_method1_t<_retT,_subjectT,_objectT,_arg1T>(subject, method);
}

template<class _retT, class _subjectT, class _objectT, class _arg1T>
inline bound_method_ref1_t<_retT,_subjectT,_objectT,_arg1T> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(_objectT&,_arg1T)) {
	return bound_method_ref1_t<_retT,_subjectT,_objectT,_arg1T>(subject, method);
}

template<class _retT, class _subjectT, class _objectT, class _arg1T>
inline bound_const_method_ref1_t<_retT,_subjectT,_objectT,_arg1T> bound_method(
	_subjectT& subject, _retT (_subjectT::* method)(const _objectT&,_arg1T)) {
	return bound_const_method_ref1_t<_retT,_subjectT,_objectT,_arg1T>(subject, method);
}

__END_CORTEX_NAMESPACE
#endif /*__functional_tools_H__*/
