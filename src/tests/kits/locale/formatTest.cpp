#include <iostream>

#include <time.h>

#include <String.h>
#include <TimeFormat.h>

int main() {
	BTimeFormat timeFormatter;
	BString str;

	timeFormatter.Format(123456, &str);

	std::cout << str.String();
}
