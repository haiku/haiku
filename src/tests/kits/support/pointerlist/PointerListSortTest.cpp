#include <ObjectList.h>
#include <String.h>

static int SortItemTestPositive(const void *item1, const void *item2)
{
	return 1;
}

static int SortItemTestNegative(const void *item1, const void *item2)
{
	return -1;
}

static int SortItemTestEqual(const void *item1, const void *item2)
{
	return 0;
}

int main(int, char **) 
{
	_PointerList_ list;
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
