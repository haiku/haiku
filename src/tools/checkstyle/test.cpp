/* Test-file for coding style checker
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT Licence
 */


// DETECTED problems


// Line longer than 80 chars
int
someveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryveryverylongFunctionName( int aLongParameter)
{
	// Identantion with spaces instead of tabs
 	int a;
	 int b;
	 	int c;

	// Missing space after control statement
	if(condition);

	//Missing space at comment start

	// Operator without spaces
	if (a>b);

	// Operator at end of line
	if (a >
		b)
	j =
		k +
		i;

	// Wrong line breaks around else
	if (test)
	{
	}
	else
	{
	}
	// Less than two lines between functions
}

// Missing space before opening brace
int
aFunction(char param){
	// More than two lines between blocks
}



// CORRECT things that should not be detected as violations
#include <dir/path.h>
	// Not matched by 'operator at end of line'


// Below this are FALSE POSITIVES (think of it as a TODO list)
// Test-file
	// Matched by 'space around operator' (should not be matched in comments)


// NOT DETECTED violations
int func()
{
	if (a)
	{
		// The brace should be on the same line as the if
	}

	// Everything related to naming conventions
}
