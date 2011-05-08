/*
 * "xmlppd.c"
 *
 * PPD to XML converter.
 *
 * Copyright 2007 by Michael R Sweet and Robert Krawitz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <stdio.h>
#include <gutenprint/mxml.h>
#include <gutenprint/util.h>
#include <gutenprint/string-list.h>
#include <stdlib.h>
#include "xmlppd.h"

typedef struct
{
  int order;
  const char *name;
} order_t;

static int
order_compare(const void *a, const void *b)
{
  const order_t *aa = (const order_t *)a;
  const order_t *bb = (const order_t *)b;
  if (aa->order < bb->order)
    return -1;
  else if (aa->order > bb->order)
    return 1;
  else
    return strcmp(aa->name, bb->name);
}


static stp_mxml_node_t *
find_element_named(stp_mxml_node_t *root, const char *name, const char *what)
{
  stp_mxml_node_t *element;
  if (root && name)
    {
      for (element = stp_mxmlFindElement(root, root, what, NULL, NULL,
					 STP_MXML_DESCEND);
	   element;
	   element = stp_mxmlFindElement(element, root, what, NULL, NULL,
					 STP_MXML_DESCEND))
	{
	  if (!strcmp(stp_mxmlElementGetAttr(element, "name"), name))
	    return element;
	}
    }
  return NULL;
}

static stp_mxml_node_t *
find_element_index(stp_mxml_node_t *root, int idx, const char *what)
{
  stp_mxml_node_t *element;
  int count = 0;
  if (root && idx >= 0)
    {
      for (element = stp_mxmlFindElement(root, root, what, NULL, NULL,
					 STP_MXML_DESCEND);
	   element;
	   element = stp_mxmlFindElement(element, root, what, NULL, NULL,
					 STP_MXML_DESCEND))
	{
	  if (count++ == idx)
	    return element;
	}
    }
  return NULL;
}

static size_t
find_element_count(stp_mxml_node_t *root, const char *what)
{
  stp_mxml_node_t *element;
  size_t count = 0;
  if (root)
    {
      for (element = stp_mxmlFindElement(root, root, what, NULL, NULL,
					 STP_MXML_DESCEND);
	   element;
	   element = stp_mxmlFindElement(element, root, what, NULL, NULL,
					 STP_MXML_DESCEND))
	count++;
    }
  return count;
}

stp_mxml_node_t *
stpi_xmlppd_find_group_named(stp_mxml_node_t *root, const char *name)
{
  return find_element_named(root, name, "group");
}

stp_mxml_node_t *
stpi_xmlppd_find_group_index(stp_mxml_node_t *root, int idx)
{
  return find_element_index(root, idx, "group");
}

int
stpi_xmlppd_find_group_count(stp_mxml_node_t *root)
{
  return find_element_count(root, "group");
}

stp_mxml_node_t *
stpi_xmlppd_find_option_named(stp_mxml_node_t *root, const char *name)
{
  return find_element_named(root, name, "option");
}

stp_mxml_node_t *
stpi_xmlppd_find_option_index(stp_mxml_node_t *root, int idx)
{
  return find_element_index(root, idx, "option");
}

int
stpi_xmlppd_find_option_count(stp_mxml_node_t *root)
{
  return find_element_count(root, "option");
}

stp_mxml_node_t *
stpi_xmlppd_find_choice_named(stp_mxml_node_t *option, const char *name)
{
  return find_element_named(option, name, "choice");
}

stp_mxml_node_t *
stpi_xmlppd_find_choice_index(stp_mxml_node_t *option, int idx)
{
  return find_element_index(option, idx, "choice");
}

int
stpi_xmlppd_find_choice_count(stp_mxml_node_t *option)
{
  return find_element_count(option, "choice");
}

stp_mxml_node_t *
stpi_xmlppd_find_page_size(stp_mxml_node_t *root, const char *name)
{
  return stpi_xmlppd_find_choice_named(stpi_xmlppd_find_option_named(root, "PageSize"), name);
}

static void
parse_values(const char **data, int limit, char *value)
{
  int dptr = 0;
  char *where = value;
  char *end = value;
  for (dptr = 0; dptr < limit; dptr++)
    data[dptr] = NULL;
  for (dptr = 0; *where && dptr < limit; dptr++)
    {
      where = end;
      while (*where && isspace(*where))
	where++;
      end = where;
      while (*end && !isspace(*end))
	end++;
      *end++ = '\0';
      data[dptr] = where;
    }
}

/*
 * 'read_ppd_file()' - Read a PPD file into XML data.
 */

stp_mxml_node_t *				/* O - PPD file as XML */
stpi_xmlppd_read_ppd_file(const char *filename)	/* I - PPD file */
{
  stp_mxml_node_t *ppd,			/* Root node of "ppd" group */
		*group,			/* Current group */
		*option,		/* Current option */
		*choice;		/* Current choice */
  FILE		*fp;			/* PPD file */
  int		ch,			/* Current character */
		sawcolon,		/* Saw a colon? */
    		inquote,		/* In a quoted string? */
    		num_choices = 0;
  char		buffer[32768],		/* Line buffer */
		*bufptr,		/* Pointer into line */
		*xptr,		/* Pointer into line */
		option_name[42],	/* Option name */
		stp_option_data_name[64],	/* Option name */
		*keyword,		/* Pointer to option keyword */
		*text,			/* Pointer to text */
		*value;			/* Pointer to value */
  order_t	*order_array;		/* Precedence order of options */
  int		i;
  int		option_count;
  int		order_length;
  char		*order_list;
  int		in_comment;
  stp_string_list_t *ialist = stp_string_list_create();
  stp_string_list_t *pdlist = stp_string_list_create();


 /*
  * Open the file...
  */

  if ((fp = fopen(filename, "rb")) == NULL)
    {
      perror(filename);
      return (NULL);
    }

 /*
  * Create the base PPD file tree; the completed tree will look like:
  *
  * <?xml version="1.0"?>
  * <ppd color="0/1" level="1/2/3">
  *     <option name="..." text="..." default="..." section="..." order="..."
  *             type="...">
  *         <choice name="..." text="...">code</choice>
  *         ...
  *     </option>
  *     ...
  *     <group name="..." text="...">
  *         <option ...>
  *              <choice ...>code</choice>
  *         </option>
  *     </group>
  * </ppd>
  */

/*  xml = stp_mxmlNewXML("1.0"); */
/*  ppd = stp_mxmlNewElement(xml, "ppd"); */
  ppd = stp_mxmlNewElement(STP_MXML_NO_PARENT, "ppd");

  stp_mxmlElementSetAttr(ppd, "color", "0");
  stp_mxmlElementSetAttr(ppd, "level", "2");

 /*
  * Read all of the lines of the form:
  *
  *   *% comment
  *   *Keyword: value
  *   *Keyword OptionKeyword: value
  *   *Keyword OptionKeyword/Text: value
  *
  * Only save groups, options, and choices, along with specific metadata
  * from the file...
  */

  group          = NULL;
  option         = NULL;
  option_name[0] = '\0';

  while ((ch = getc(fp)) != EOF)
    {
      /*
       * Read the line...
       */

      buffer[0] = ch;
      bufptr    = buffer + 1;
      inquote   = 0;
      sawcolon  = 0;
      in_comment = 0;
      if (ch == '*')
	{
	  if ((ch = getc(fp)) == '%')
	    {
	      while (1)
		{
		  ch = getc(fp);
		  if (ch == '\n' || ch == EOF)
		    break;
		}
	      continue;
	    }
	  else
	    ungetc(ch, fp);
	}

      while ((ch != '\n' || inquote) && bufptr < (buffer + sizeof(buffer) - 1))
	{
	  if ((ch = getc(fp)) == '\r')
	    {
	      if ((ch = getc(fp)) != '\n')
		ungetc(ch, fp);
	    }

	  *bufptr++ = ch;

	  if (ch == ':' && !sawcolon)
	    sawcolon = 1;
	  else if (ch == '\"' && sawcolon)
	    inquote = !inquote;
	}

      /*
       * Strip trailing whitespace...
       */

      for (bufptr --; bufptr > buffer && isspace(bufptr[-1] & 255); bufptr --);

      *bufptr = '\0';

      /*
       * Now parse it...
       */

      if (!strncmp(buffer, "*%", 2) || !buffer[0])
	continue;

      if ((value = strchr(buffer, ':')) == NULL)
	continue;

      for (*value++ = '\0';
	   *value && (isspace(*value & 255) || (*value & 255) == '"');
	   value ++);
      for (xptr = value;
	   *xptr && (*xptr & 255) != '"';
	   xptr ++);
      if (*xptr == '"')
	*xptr = '\0';

      for (keyword = buffer; *keyword && !isspace(*keyword & 255); keyword ++);

      while (isspace(*keyword & 255))
	*keyword++ = '\0';

      if ((text = strchr(keyword, '/')) != NULL)
	*text++ = '\0';

      /*
       * And then use the parsed values...
       */

      if (!strcmp(buffer, "*ColorDevice"))
	{
	  /*
	   * Color support...
	   */

	  if (!strcasecmp(value, "true"))
	    stp_mxmlElementSetAttr(ppd, "color", "1");
	  else
	    stp_mxmlElementSetAttr(ppd, "color", "0");
	}
      else if (!strcmp(buffer, "*LanguageLevel") && atoi(value) > 0)
	{
	  /*
	   * PostScript language level...
	   */

	  stp_mxmlElementSetAttr(ppd, "level", value);
	}
      else if (!strcmp(buffer, "*StpDriverName"))
	stp_mxmlElementSetAttr(ppd, "driver", value);
      else if (!strcmp(buffer, "*ModelName"))
	stp_mxmlElementSetAttr(ppd, "modelname", value);
      else if (!strcmp(buffer, "*ShortNickName"))
	stp_mxmlElementSetAttr(ppd, "shortnickname", value);
      else if (!strcmp(buffer, "*NickName"))
	stp_mxmlElementSetAttr(ppd, "nickname", value);
      else if (!strcmp(buffer, "*OpenGroup"))
	{
	  if ((text = strchr(value, '/')) != NULL)
	    *text++ = '\0';

	  group = stp_mxmlNewElement(ppd, "group");
	  stp_mxmlElementSetAttr(group, "name", value);
	  stp_mxmlElementSetAttr(group, "text", text ? text : value);
	}
      else if (!strcmp(buffer, "*CloseGroup"))
	group = NULL;
      else if ((!strcmp(buffer, "*OpenUI") || !strcmp(buffer, "*JCLOpenUI")) &&
	       keyword[0] == '*' && keyword[1])
	{
	  /*
	   * Start a new option...
	   */

	  option = stp_mxmlNewElement(group ? group : ppd, "option");
	  stp_mxmlElementSetAttr(option, "name", keyword + 1);
	  stp_mxmlElementSetAttr(option, "text", text ? text : keyword + 1);
	  stp_mxmlElementSetAttr(option, "ui", value);

	  strncpy(option_name, keyword, sizeof(option_name) - 1);
	  option_name[sizeof(option_name) - 1] = '\0';
	  strcpy(stp_option_data_name, "*Stp");
	  strcpy(stp_option_data_name + 4, option_name + 1);
	  if (group)
	    {
	      stp_mxmlElementSetAttr(option, "groupname", stp_mxmlElementGetAttr(group, "name"));
	      stp_mxmlElementSetAttr(option, "grouptext", stp_mxmlElementGetAttr(group, "text"));
	    }
	  num_choices = 0;
	}
      else if (option && !strcmp(buffer, stp_option_data_name))
	{
	  const char *data[8];
	  parse_values(data, 8, value);
	  if (data[7])
	    {
	      stp_mxmlElementSetAttr(option, "stptype", data[0]);
	      stp_mxmlElementSetAttr(option, "stpmandatory", data[1]);
	      stp_mxmlElementSetAttr(option, "stpclass", data[2]);
	      stp_mxmlElementSetAttr(option, "stplevel", data[3]);
	      stp_mxmlElementSetAttr(option, "stpchannel", data[4]);
	      stp_mxmlElementSetAttr(option, "stplower", data[5]);
	      stp_mxmlElementSetAttr(option, "stpupper", data[6]);
	      stp_mxmlElementSetAttr(option, "stpdefault", data[7]);
	      stp_mxmlElementSetAttr(option, "stpname", stp_option_data_name + 7);
	    }
	}
      else if (!strcmp(buffer, "*OrderDependency") && option)
	{
	  /*
	   * Get order and section for option
	   */

	  char	order[256],		/* Order number */
	    section[256];		/* Section name */


	  if (sscanf(value, "%255s%255s", order, section) == 2)
	    {
	      stp_mxmlElementSetAttr(option, "order", order);
	      stp_mxmlElementSetAttr(option, "section", section);
	    }
	}
      else if (!strncmp(buffer, "*Default", 8) && option &&
	       !strcmp(buffer + 8, option_name + 1))
	stp_mxmlElementSetAttr(option, "default", value);
      else if (!strcmp(buffer, "*CloseUI") || !strcmp(buffer, "*JCLCloseUI"))
	{
	  char buf[64];
	  (void) sprintf(buf, "%d", num_choices);
	  stp_mxmlElementSetAttr(option, "num_choices", buf);
	  option = NULL;
	  stp_option_data_name[0] = '\0';
	}
      else if (option && !strcmp(buffer, option_name))
	{
	  /*
	   * A choice...
	   */

	  choice = stp_mxmlNewElement(option, "choice");
	  stp_mxmlElementSetAttr(choice, "name", keyword);
	  stp_mxmlElementSetAttr(choice, "text", text ? text : keyword);

	  if (value[0] == '\"')
	    value ++;

	  if (bufptr > buffer && bufptr[-1] == '\"')
	    {
	      bufptr --;
	      *bufptr = '\0';
	    }

	  stp_mxmlNewOpaque(choice, value);
	  num_choices++;
	}
      else if (!option && !strcmp(buffer, "*ImageableArea"))
	{
	  stp_string_list_add_string(ialist, keyword, value);
	}
      else if (!option && !strcmp(buffer, "*PaperDimension"))
	{
	  stp_string_list_add_string(pdlist, keyword, value);
	}
    }
  if (option)
    {
      char buf[64];
      (void) sprintf(buf, "%d", num_choices);
      stp_mxmlElementSetAttr(option, "num_choices", buf);
      option = NULL;
      stp_option_data_name[0] = '\0';
    }
      
  for (i = 0; i < stp_string_list_count(ialist); i++)
    {
      stp_param_string_t *pstr = stp_string_list_param(ialist, i);
      stp_mxml_node_t *psize = stpi_xmlppd_find_page_size(ppd, pstr->name);
      if (psize)
	{
	  const char *data[4];
	  value = stp_strdup(pstr->text);
	  parse_values(data, 4, value);
	  if (data[3])
	    {
	      stp_mxmlElementSetAttr(psize, "left", data[0]);
	      stp_mxmlElementSetAttr(psize, "bottom", data[1]);
	      stp_mxmlElementSetAttr(psize, "right", data[2]);
	      stp_mxmlElementSetAttr(psize, "top", data[3]);
	    }
	  stp_free(value);
	}
    }
  stp_string_list_destroy(ialist);
  for (i = 0; i < stp_string_list_count(pdlist); i++)
    {
      stp_param_string_t *pstr = stp_string_list_param(pdlist, i);
      stp_mxml_node_t *psize = stpi_xmlppd_find_page_size(ppd, pstr->name);
      if (psize)
	{
	  const char *data[2];
	  value = stp_strdup(pstr->text);
	  parse_values(data, 2, value);
	  if (data[1])
	    {
	      stp_mxmlElementSetAttr(psize, "width", data[0]);
	      stp_mxmlElementSetAttr(psize, "height", data[1]);
	    }
	  stp_free(value);
	}
    }
  stp_string_list_destroy(pdlist);
  option_count = stpi_xmlppd_find_option_count(ppd);
  order_length = 1;		/* Terminating null */
  order_array = malloc(sizeof(order_t) * option_count);
  i = 0;
  for (option = stp_mxmlFindElement(ppd, ppd, "option", NULL, NULL,
				    STP_MXML_DESCEND);
       option && i < option_count;
       option = stp_mxmlFindElement(option, ppd, "option", NULL, NULL,
				    STP_MXML_DESCEND))
    {
      if (stp_mxmlElementGetAttr(option, "order"))
	{
	  order_array[i].name = stp_mxmlElementGetAttr(option, "name");
	  order_length += strlen(order_array[i].name) + 1;
	  order_array[i].order = atoi(stp_mxmlElementGetAttr(option, "order"));
	  i++;
	}
    }
  option_count = i;
  qsort(order_array, option_count, sizeof(order_t), &order_compare);
  order_list = malloc(order_length);
  order_length = 0;
  for (i = 0; i < option_count; i++)
    {
      if (i > 0)
	order_list[order_length++] = ' ';
      strcpy(order_list + order_length, order_array[i].name);
      order_length += strlen(order_array[i].name);
    }
  stp_mxmlElementSetAttr(ppd, "optionorder", order_list);
  free(order_list);
  free(order_array);
  return (ppd);
}

/*
 * End of "xmlppd.c".
 */
