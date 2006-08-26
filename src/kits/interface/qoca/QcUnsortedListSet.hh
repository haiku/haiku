#ifndef __QcUnsortedListSetH
#define __QcUnsortedListSetH

#include <vector.h>

template <class T, class Alloc = alloc>
class QcUnsortedListSet
{
public:
	typedef T value_type;
	typedef value_type* iterator;
	typedef value_type const * const_iterator;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef size_t size_type;

public:
	QcUnsortedListSet()
		: elements()
	{
	}

	QcUnsortedListSet(int initCapacity)
		: elements(initCapacity)
	{
	}

	QcUnsortedListSet(int initCapacity, int increment)
		: elements(initCapacity, increment)
	{
	}

	iterator begin() { return elements.begin(); }
	iterator end() { return elements.end(); }

	iterator abegin() { return elements.begin(); }
	iterator aend() { return elements.end(); }

	const_iterator abegin() const { return elements.begin(); }
	const_iterator aend() const { return elements.end(); }

	void clear()
	{
		elements.clear();
	}

	iterator erase(iterator position)
	{
		if (position + 1 < elements.end())
			*position = elements.back();
		elements.pop_back();
		return position;
	}

	void push_back(const T& x)
	{
		elements.push_back(x);
	}

	reference operator[](size_type n)
		{ return elements[n]; }

	const_reference operator[](size_type n) const
		{ return elements[n]; }

	void resize(size_type new_size)
	{
		elements.resize(new_size);
	}

	size_type size() const
	{
		return elements.size();
	}

private:
	/* effic: We should allow a version implemented as a subclass instead of
	   using the extra indirection of an instance variable.

	   (The reason for using an instance variable is so that we can control
	   the interface.) */
	vector<T> elements;
};
#endif /* !__QcUnsortedListSetH */
