#include <List.h>
#include <String.h>

class NestedString {
	public:
		NestedString();
		~NestedString();
		
		NestedString &operator [] (int) const;
		const char *operator ()() const;
		
		NestedString &AdoptAndAdd(const char *);
		NestedString &operator += (const char *);
		NestedString &operator += (BString &);
		
		int32 CountItems() const;
		bool HasChildren() const;
		
		void PrintToStream(int indent = 0);
	private:
		BList children;
		const char *string;
		bool we_own;
};
