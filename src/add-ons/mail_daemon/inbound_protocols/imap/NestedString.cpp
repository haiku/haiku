#include <stdio.h>

#include "NestedString.h"

NestedString::NestedString() : string(NULL), we_own(false) {}

NestedString::~NestedString() {
	for (int32 i = 0; i < children.CountItems(); i++)
		delete (NestedString *)(children.ItemAt(i));
		
	if (we_own && string)
		delete [] string;
}

NestedString &NestedString::operator [] (int i) const {	
	return *((NestedString *)(children.ItemAt(i)));
}

const char *NestedString::operator ()() const {
	if (string == NULL) {
		static BString result; //---This isn't at all thread-safe, but it doesn't matter, since it's single-threaded
		result = "(";
		for (int32 i = 0; i < CountItems(); i++)
			result << (*this)[i]() << ' ';
		result << ')';
		return result.String();
	}
	return string;
}

NestedString &NestedString::AdoptAndAdd(const char *add) {
	
	if (string != NULL) {
		NestedString *to_add = new NestedString;
		to_add->string = string;
		to_add->we_own = we_own;
		children.AddItem(to_add);
		string = NULL;
	}
	
	NestedString *to_add = new NestedString;
	to_add->string = add;
	to_add->we_own = true;
	children.AddItem(to_add);
	return *to_add;
}
	
NestedString &NestedString::operator += (const char *add) {
	//printf("Adding string: \"%s\"\n",add);
	
	if (string != NULL) {
		NestedString *to_add = new NestedString;
		to_add->string = string;
		to_add->we_own = we_own;
		children.AddItem(to_add);
		string = NULL;
	}
	
	NestedString *to_add = new NestedString;
	to_add->string = add;
	to_add->we_own = false;
	children.AddItem(to_add);
	return *to_add;
}

NestedString &NestedString::operator += (BString &add) {
	//printf("Adding string: \"%s\"\n",add.String());

	if (string != NULL) {
		NestedString *to_add = new NestedString;
		to_add->string = string;
		to_add->we_own = we_own;
		children.AddItem(to_add);
		string = NULL;
	}
	
	NestedString *to_add = new NestedString;
	to_add->string = new char[add.Length()+1];
	strcpy((char *)(to_add->string),add.String());
	to_add->we_own = false;
	children.AddItem(to_add);
	return *to_add;
}

int32 NestedString::CountItems() const {
	return children.CountItems();
}

bool NestedString::HasChildren() const {
	return (children.CountItems() != 0);
}

void NestedString::PrintToStream(int indentation) {
	for (int j = 0; j < indentation; j++)
			printf("\t");
	printf("%d items:\n",CountItems());
	/*if (CountItems() == 1) {
		for (int j = 0; j < indentation; j++)
			printf("\t");
		printf("0: %s",(*this)());
	}*/
	for (int32 i = 0; i < CountItems(); i++) {
		for (int j = 0; j < indentation; j++)
			printf("\t");
		if ((*this)[i].HasChildren())
			(*this)[i].PrintToStream(indentation+1);
		else
			printf("%d: %s\n",i,(*this)[i]());
	}
}

