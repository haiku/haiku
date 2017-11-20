/*
 * Copyright 2017, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		 <François Revol>
 */

#include <stdio.h>

#include <driver_settings.h>

static const char *sTabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

int usage(const char *progname)
{
	return 1;
}

void print_params(int indent, const driver_parameter *p)
{
	printf("indent: %d\n", indent);
	printf("%.*s'%s': [", indent, sTabs, p->name);
	for (int i = 0; i < p->value_count; i++) {
		printf(" '%s',", p->values[i]);
	}
	printf("]\n");
	indent++;
	for (int i = 0; i < p->parameter_count; i++)
		print_params(indent, &p->parameters[i]);
}

int main(int argc, char **argv)
{
	void *h;
	const driver_settings *s;
	h = load_driver_settings(argv[1]);
	if (h == NULL)
		return usage(argv[0]);

	s = get_driver_settings(h);

	printf("%d\n", s->parameter_count);
	for (int i = 0; i < s->parameter_count; i++) {
		print_params(0, &s->parameters[i]);
	}

	unload_driver_settings(h);

	return 0;
}
