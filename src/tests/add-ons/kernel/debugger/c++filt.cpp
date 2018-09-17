/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Based on Demumble; Copyright 2016-2018, Nico Weber.
 * 		https://github.com/nico/demumble/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "Demangler.h"


static void print_help(FILE* out)
{
	fprintf(out,
		"usage: haikuc++filt [options] [symbols...]\n"
		"\n"
		"if symbols are unspecified, reads from stdin.\n"
		"\n"
		"options:\n"
		"  -m         only print mangled names that were demangled,"
			"omit other output\n"
		"  -u         use unbuffered output\n"
		"  --no-gcc2	ignore GCC 2-style symbols\n");
}


static bool starts_with(const char* s, const char* prefix)
{
	return strncmp(s, prefix, strlen(prefix)) == 0;
}


static void print_demangled(const char* s)
{
	const char* cxa_in = s;
	if (starts_with(s, "__Z") || starts_with(s, "____Z"))
		cxa_in += 1;
	printf("%s", Demangler::Demangle(cxa_in).String());
}


static bool is_mangle_char_posix(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') || c == '_';
}


static bool look_for_itanium_prefix(char** str, char* end)
{
	char* s = *str;
	s += strcspn(s, "_?");
	if (s == end)
		return false;

	// Itanium symbols start with 1-4 underscores followed by Z.
	// strnstr() is BSD, so use a small local buffer and strstr().
	const int N = 5;  // == strlen("____Z")
	char prefix[N + 1];
	strncpy(prefix, s, N);
		prefix[N] = '\0';
	if (strstr(prefix, "_Z")) {
		*str = s;
		return true;
	}
	return false;
}


static bool look_for_gcc2_symbol(char** str, char* end)
{
	// search '__' starting from the end, don't accept them at the start
	char* s = *str;
	size_t pos = (end - s) - 1;
	char* mangled = NULL;

	while (pos > 1) {
		if (s[pos] == '_') {
			if (s[pos - 1] == '_') {
				mangled = s + pos + 1;
				break;
			} else
				pos--;
		}
		pos--;
	}

	// if we've found a symbol, go backwards to its beginning
	while (mangled != NULL && mangled > (s + 1)
			&& is_mangle_char_posix(mangled[-1])) {
		mangled--;
	}

	if (mangled != NULL)
		*str = mangled;

	return mangled != NULL;
}


static char buf[8192];
int main(int argc, char* argv[])
{
	enum { kPrintAll, kPrintMatching } print_mode = kPrintAll;
	bool noGCC2 = false;

	while (argc > 1 && argv[1][0] == '-') {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			print_help(stdout);
			return 0;
		} else if (strcmp(argv[1], "-m") == 0) {
			print_mode = kPrintMatching;
		} else if (strcmp(argv[1], "--no-gcc2") == 0) {
			noGCC2 = true;
		} else if (strcmp(argv[1], "-u") == 0) {
			setbuf(stdout, NULL);
		} else if (strcmp(argv[1], "--") == 0) {
			--argc;
			++argv;
			break;
		} else {
			fprintf(stderr, "c++filt: unrecognized option `%s'\n", argv[1]);
			print_help(stderr);
			return 1;
		}
		--argc;
		++argv;
	}
	for (int i = 1; i < argc; ++i) {
		print_demangled(argv[i]);
		printf("\n");
	}
	if (argc != 1)
		return 0;
	// Read stdin instead.
	// By default, don't demangle types.  Mangled function names are unlikely
	// to appear in text for since they start with _Z (or ___Z) or ?? / ?$ / ?@.
	// But type manglings can be regular words ("Pi" is "int*").
	// (For command-line args, do try to demangle types though.)
	while (fgets(buf, sizeof(buf), stdin)) {
		bool need_separator = false;
		char* cur = buf;
		char* end = cur + strlen(cur);

		while (cur != end) {
			if (print_mode == kPrintMatching && need_separator)
				printf("\n");
			need_separator = false;

			// Check if we have a symbol, and then for how long it is.
			size_t n_sym = 0;
			char* real_cur = cur;
			if (look_for_itanium_prefix(&real_cur, end) ||
					(!noGCC2 && look_for_gcc2_symbol(&real_cur, end))) {
				// Print all the stuff before the symbol.
				if (print_mode == kPrintAll)
					printf("%.*s", static_cast<int>(real_cur - cur), cur);
				cur = real_cur;
				while (cur + n_sym != end && is_mangle_char_posix(cur[n_sym]))
					++n_sym;
			} else {
				// No symbols found in this block; skip it.
				printf("%s", cur);
				cur = end;
				continue;
			}
			if (n_sym == 0) {
				++cur;
				continue;
			}

			char tmp = cur[n_sym];
			cur[n_sym] = '\0';
			print_demangled(cur);
			need_separator = true;
			cur[n_sym] = tmp;

			cur += n_sym;
		}
	}
}
