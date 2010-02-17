#include <iostream>

#include <time.h>

#include <String.h>
#include <TimeFormat.h>

int
main()
{
	BTimeFormat timeFormatter;
	BString str;

	if (timeFormatter.Format(123456, &str) != B_OK) {
		std::cout << "Conversion error\n";
		return -1;
	}

	std::cout << str.String() << std::endl;
	return 0;
}
