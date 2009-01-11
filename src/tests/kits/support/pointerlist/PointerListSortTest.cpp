#include <ObjectList.h>
#include <String.h>

static int SortItemTestPositive(const BString *item1, const BString *item2)
{
	return 1;
}

static int SortItemTestNegative(const BString *item1, const BString *item2)
{
	return -1;
}

static int SortItemTestEqual(const BString *item1, const BString *item2)
{
	return 0;
}

int main(int, char **) 
{
	BObjectList<BString> list;
	for (int i = 0; i < 20; i++) {
		list.AddItem(new BString("test"));
		printf("List contains %d items, attempting sorts\n", i);
		printf("Attempting positive test\n");
		list.SortItems(SortItemTestPositive);
		printf("Positive test completed, attempting negative test\n");
		list.SortItems(SortItemTestNegative);
		printf("Positive test completed, attempting equal test\n");
		list.SortItems(SortItemTestEqual);
	}
	printf("All tests passed!\n");
	
	return 0;
}
