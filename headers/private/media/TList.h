#ifndef _MEDIA_T_LIST_H
#define _MEDIA_T_LIST_H

template<class value> class List
{
public:
	List() : count(0) {}
	
	void Insert(const value &v)
	{
		value temp;
		if (count == MAXENT) debugger("template List out of memory");
		list[count] = v;
		count++;
	}
	
	// you can't Remove() while iterating through the list using GetAt()
	bool GetAt(int32 index, value *v)
	{
		if (index < 0 || index >= count) 
			return false;
		*v = list[index];
		return true;
	}

	bool GetPointerAt(int32 index, value **v)
	{
		if (index < 0 || index >= count) 
			return false;
		*v = &list[index];
		return true;
	}
	
	// you can't Remove() while iterating through the map using GetAt()
	bool Remove(int32 index) 
	{
		if (index < 0 || index >= count) 
			return false;
		count--;
		if (count > 0)
			list[index] = list[count];
		return true;
	}
	
	void MakeEmpty()
	{
		count = 0;
	}

private:
	enum { MAXENT = 64 };
	value list[MAXENT];
	int count;
};

#endif // _MEDIA_T_LIST_H
