/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_export.c
 * interface between assembly code and the application
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_export.h"

/*
 * slang_export_data_quant
 */

GLvoid slang_export_data_quant_ctr (slang_export_data_quant *self)
{
	self->name = SLANG_ATOM_NULL;
	self->size = 0;
	self->array_len = 0;
	self->structure = NULL;
	self->u.basic_type = GL_FLOAT;
}

GLvoid slang_export_data_quant_dtr (slang_export_data_quant *self)
{
	if (self->structure != NULL)
	{
		GLuint i;

		for (i = 0; i < self->u.field_count; i++)
			slang_export_data_quant_dtr (&self->structure[i]);
		slang_alloc_free (self->structure);
	}
}

slang_export_data_quant *slang_export_data_quant_add_field (slang_export_data_quant *self)
{
	const GLuint n = self->u.field_count;

	self->structure = (slang_export_data_quant *) slang_alloc_realloc (self->structure,
		n * sizeof (slang_export_data_quant), (n + 1) * sizeof (slang_export_data_quant));
	if (self->structure == NULL)
		return NULL;
	slang_export_data_quant_ctr (&self->structure[n]);
	self->u.field_count++;
	return &self->structure[n];
}

GLboolean slang_export_data_quant_array (slang_export_data_quant *self)
{
	return self->array_len != 0;
}

GLboolean slang_export_data_quant_struct (slang_export_data_quant *self)
{
	return self->structure != NULL;
}

GLboolean slang_export_data_quant_simple (slang_export_data_quant *self)
{
	return self->array_len == 0 && self->structure == NULL;
}

GLenum slang_export_data_quant_type (slang_export_data_quant *self)
{
	assert (self->structure == NULL);
	return self->u.basic_type;
}

GLuint slang_export_data_quant_fields (slang_export_data_quant *self)
{
	assert (self->structure != NULL);
	return self->u.field_count;
}

GLuint slang_export_data_quant_elements (slang_export_data_quant *self)
{
	if (self->array_len == 0)
		return 1;
	return self->array_len;
}

GLuint slang_export_data_quant_components (slang_export_data_quant *self)
{
	return self->size / 4;
}

GLuint slang_export_data_quant_size (slang_export_data_quant *self)
{
	return self->size;
}

/*
 * slang_export_data_entry
 */

GLvoid slang_export_data_entry_ctr (slang_export_data_entry *self)
{
	slang_export_data_quant_ctr (&self->quant);
	self->access = slang_exp_uniform;
	self->address = ~0;
}

GLvoid slang_export_data_entry_dtr (slang_export_data_entry *self)
{
	slang_export_data_quant_dtr (&self->quant);
}

/*
 * slang_export_data_table
 */

GLvoid slang_export_data_table_ctr (slang_export_data_table *self)
{
	self->entries = NULL;
	self->count = 0;
	self->atoms = NULL;
}

GLvoid slang_export_data_table_dtr (slang_export_data_table *self)
{
	if (self->entries != NULL)
	{
		GLuint i;

		for (i = 0; i < self->count; i++)
			slang_export_data_entry_dtr (&self->entries[i]);
		slang_alloc_free (self->entries);
	}
}

slang_export_data_entry *slang_export_data_table_add (slang_export_data_table *self)
{
	const GLuint n = self->count;

	self->entries = (slang_export_data_entry *) slang_alloc_realloc (self->entries,
		n * sizeof (slang_export_data_entry), (n + 1) * sizeof (slang_export_data_entry));
	if (self->entries == NULL)
		return NULL;
	slang_export_data_entry_ctr (&self->entries[n]);
	self->count++;
	return &self->entries[n];
}

/*
 * slang_export_code_entry
 */

static GLvoid slang_export_code_entry_ctr (slang_export_code_entry *self)
{
	self->name = SLANG_ATOM_NULL;
	self->address = ~0;
}

static GLvoid slang_export_code_entry_dtr (slang_export_code_entry *self)
{
}

/*
 * slang_export_code_table
 */

GLvoid slang_export_code_table_ctr (slang_export_code_table *self)
{
	self->entries = NULL;
	self->count = 0;
	self->atoms = NULL;
}

GLvoid slang_export_code_table_dtr (slang_export_code_table *self)
{
	if (self->entries != NULL)
	{
		GLuint i;

		for (i = 0; i < self->count; i++)
			slang_export_code_entry_dtr (&self->entries[i]);
		slang_alloc_free (self->entries);
	}
}

slang_export_code_entry *slang_export_code_table_add (slang_export_code_table *self)
{
	const GLuint n = self->count;

	self->entries = (slang_export_code_entry *) slang_alloc_realloc (self->entries,
		n * sizeof (slang_export_code_entry), (n + 1) * sizeof (slang_export_code_entry));
	if (self->entries == NULL)
		return NULL;
	slang_export_code_entry_ctr (&self->entries[n]);
	self->count++;
	return &self->entries[n];
}

/*
 * _slang_find_exported_data()
 */

#define EXTRACT_ERROR 0
#define EXTRACT_BASIC 1
#define EXTRACT_ARRAY 2
#define EXTRACT_STRUCT 3
#define EXTRACT_STRUCT_ARRAY 4

#define EXTRACT_MAXLEN 255

static GLuint extract_name (const char *name, char *parsed, GLuint *element, const char **end)
{
	GLuint i;

	if ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z') || name[0] == '_')
	{
		parsed[0] = name[0];

		for (i = 1; i < EXTRACT_MAXLEN; i++)
		{
			if ((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z') ||
				(name[i] >= '0' && name[i] <= '9') || name[0] == '_')
			{
				parsed[i] = name[i];
			}
			else
			{
				if (name[i] == '\0')
				{
					parsed[i] = '\0';
					return EXTRACT_BASIC;
				}
				if (name[i] == '.')
				{
					parsed[i] = '\0';
					*end = &name[i + 1];
					return EXTRACT_STRUCT;
				}
				if (name[i] == '[')
				{
					parsed[i] = '\0';
					i++;
					if (name[i] >= '0' && name[i] <= '9')
					{
						*element = name[i] - '0';
						for (i++; ; i++)
						{
							if (name[i] >= '0' && name[i] <= '9')
								*element = *element * 10 + (name[i] - '0');
							else
							{
								if (name[i] == ']')
								{
									i++;
									if (name[i] == '.')
									{
										*end = &name[i + 1];
										return EXTRACT_STRUCT_ARRAY;
									}
									*end = &name[i];
									return EXTRACT_ARRAY;
								}
								break;
							}
						}
					}
				}
				break;
			}
		}
	}
	return EXTRACT_ERROR;
}

static GLboolean validate_extracted (slang_export_data_quant *q, GLuint element, GLuint extr)
{
	switch (extr)
	{
	case EXTRACT_BASIC:
		return GL_TRUE;
	case EXTRACT_ARRAY:
		return element < slang_export_data_quant_elements (q);
	case EXTRACT_STRUCT:
		return slang_export_data_quant_struct (q);
	case EXTRACT_STRUCT_ARRAY:
		return slang_export_data_quant_struct (q) && element < slang_export_data_quant_elements (q);
	}
	return GL_FALSE;
}

static GLuint calculate_offset (slang_export_data_quant *q, GLuint element)
{
	if (slang_export_data_quant_array (q))
		return element * slang_export_data_quant_size (q);
	return 0;
}

static GLboolean find_exported_data (slang_export_data_quant *q, const char *name,
	slang_export_data_quant **quant, GLuint *offset, slang_atom_pool *atoms)
{
	char parsed[EXTRACT_MAXLEN];
	GLuint result, element, i;
	const char *end;
	slang_atom atom;
	const GLuint fields = slang_export_data_quant_fields (q);

	result = extract_name (name, parsed, &element, &end);
	if (result == EXTRACT_ERROR)
		return GL_FALSE;

	atom = slang_atom_pool_atom (atoms, parsed);
	if (atom == SLANG_ATOM_NULL)
		return GL_FALSE;

	for (i = 0; i < fields; i++)
		if (q->structure[i].name == atom)
		{
			if (!validate_extracted (&q->structure[i], element, result))
				return GL_FALSE;
			*offset += calculate_offset (&q->structure[i], element);
			if (result == EXTRACT_BASIC || result == EXTRACT_ARRAY)
			{
				if (*end != '\0')
					return GL_FALSE;
				*quant = &q->structure[i];
				return GL_TRUE;
			}
			return find_exported_data (&q->structure[i], end, quant, offset, atoms);
		}
	return GL_FALSE;
}

GLboolean _slang_find_exported_data (slang_export_data_table *table, const char *name,
	slang_export_data_entry **entry, slang_export_data_quant **quant, GLuint *offset)
{
	char parsed[EXTRACT_MAXLEN];
	GLuint result, element, i;
	const char *end;
	slang_atom atom;

	result = extract_name (name, parsed, &element, &end);
	if (result == EXTRACT_ERROR)
		return GL_FALSE;

	atom = slang_atom_pool_atom (table->atoms, parsed);
	if (atom == SLANG_ATOM_NULL)
		return GL_FALSE;

	for (i = 0; i < table->count; i++)
		if (table->entries[i].quant.name == atom)
		{
			if (!validate_extracted (&table->entries[i].quant, element, result))
				return GL_FALSE;
			*entry = &table->entries[i];
			*offset = calculate_offset (&table->entries[i].quant, element);
			if (result == EXTRACT_BASIC || result == EXTRACT_ARRAY)
			{
				if (*end != '\0')
					return GL_FALSE;
				*quant = &table->entries[i].quant;
				return GL_TRUE;
			}
			return find_exported_data (&table->entries[i].quant, end, quant, offset, table->atoms);
		}
	return GL_FALSE;
}

