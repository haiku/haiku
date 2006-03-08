// set_tools.h
// e.moon 7may99
//
// PURPOSE
//   Tools to manipulate STL set types.
//
// HISTORY
//   e.moon 27jul99		moved into cortex namespace
//   e.moon	7may99		created

#ifndef __SET_TOOLS_H__
#define __SET_TOOLS_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// delete range of pointer values from set
template<class iter>
void ptr_set_delete(iter begin, iter end) {
	while(begin != end) {
		if(*begin)
			delete *begin;
		++begin;
	}
}

// delete range of pointer values from map
template<class iter>
void ptr_map_delete(iter begin, iter end) {
	while(begin != end) {
		if((*begin).second)
			delete (*begin).second;
		++begin;
	}
}

// a simple equality-test functor for maps
template<class key, class value>
class map_value_equal_to :
	public binary_function<pair<key,value>, value, bool> {

public:
	bool operator()(const pair<key,value>& p, const value& v) const {
		return p.second == v;
	}
};

//// a predicate functor adaptor for maps
//// e.moon 28jul99
//template<class key, class value>
//class map_value_predicate_t :
//	public unary_function<pair<key,value>, bool> {
//
//	const unary_function<const value, bool>& fn;
//
//public:
//	map_value_predicate_t(const unary_function<const value, bool>& _fn) : fn(_fn) {}
//	bool operator()(const pair<key,value>& p) const {
//		return fn(p.second);
//	}
//};
//
//template<class key, class value>
//inline map_value_predicate_t<key,value> map_value_predicate(
//	const unary_function<const value, bool>& fn) {
//	return map_value_predicate_t<key,value>(fn);
//}

// copy values from a map subset
template<class input_iter, class output_iter>
void map_value_copy(input_iter begin, input_iter end, output_iter to) {
	while(begin != end) {
		*to = (*begin).second;
		++to;
		++begin;
	}
}

// adapt a unary functor to a map (eek)
template <class pairT, class opT>
class unary_map_function_t :
	public unary_function<typename opT::argument_type, typename opT::result_type> {
	
	opT f;

public:
	unary_map_function_t(const opT& _f) : f(_f) {}

	typename opT::result_type
	operator()(pairT& p) const {
		return f(p.second);
	}
};

template <class mapT, class opT>
inline unary_map_function_t<typename mapT::value_type, opT>
unary_map_function(
	const mapT& map,
	const opT& f) {
	return unary_map_function_t<typename mapT::value_type, opT>(f);
}


__END_CORTEX_NAMESPACE
#endif /* __SET_TOOLS_H__ */