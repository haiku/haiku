
template<class key, class value> class Map
{
public:
	Map() : count(0) {}
	
	void Insert(const key &k, const value &v)
	{
		value temp;
		if (count == MAXENT) debugger("template Map out of memory");
		if (Get(k, &temp)) debugger("template Map inserting duplicate key");
		list[count].k = k;
		list[count].v = v;
		count++;
	}
	
	bool Get(const key &k, value *v)
	{
		for (int i = 0; i < count; i++)
			if (list[i].k == k) {
				*v = list[i].v;
				return true;
			}
		return false;
	}
	
	// you can't Remove() while iterating through the map using GetAt()
	bool GetAt(int32 index, value *v)
	{
		if (index < 0 || index >= count) 
			return false;
		*v = list[index].v;
		return true;
	}
	
	// you can't Remove() while iterating through the map using GetAt()
	bool Remove(const key &k) 
	{
		for (int i = 0; i < count; i++)
			if (list[i].k == k) {
				count--;
				if (count > 0) {
					list[i].v = list[count].v;
					list[i].k = list[count].k;
				}
				return true;
			}
		return false;
	}

private:
	enum { MAXENT = 64 };
	struct ent {
		key k;
		value v;
	};
	ent list[MAXENT];
	int count;
};

