/*
 * "$Id: mxml-file.c,v 1.10 2008/07/20 01:12:15 easysw Exp $"
 *
 * File loading code for mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contents:
 *
 *   stp_mxmlLoadFile()        - Load a file into an XML node tree.
 *   stp_mxmlLoadFromFile()    - Load a file into an XML node tree.
 *   stp_mxmlLoadString()      - Load a string into an XML node tree.
 *   stp_mxmlSaveAllocString() - Save an XML node tree to an allocated string.
 *   stp_mxmlSaveFile()        - Save an XML tree to a file.
 *   stp_mxmlSaveString()      - Save an XML node tree to a string.
 *   mxml_add_char()       - Add a character to a buffer, expanding as needed.
 *   mxml_file_getc()      - Get a character from a file.
 *   mxml_load_data()      - Load data into an XML node tree.
 *   mxml_parse_element()  - Parse an element for any attributes...
 *   mxml_string_getc()    - Get a character from a string.
 *   mxml_write_node()     - Save an XML node to a file.
 *   mxml_write_string()   - Write a string, escaping & and < as needed.
 *   mxml_write_ws()       - Do whitespace callback...
 */

/*
 * Include necessary headers...
 */

#include <gutenprint/mxml.h>
#include "config.h"


/*
 * Local functions...
 */

static int		mxml_add_char(int ch, char **ptr, char **buffer,
			              int *bufsize);
static int		mxml_file_getc(void *p);
static int		mxml_file_putc(int ch, void *p);
static stp_mxml_node_t	*mxml_load_data(stp_mxml_node_t *top, void *p,
			                stp_mxml_type_t (*cb)(stp_mxml_node_t *),
			                int (*getc_cb)(void *));
static int		mxml_parse_element(stp_mxml_node_t *node, void *p,
			                   int (*getc_cb)(void *));
static int		mxml_string_getc(void *p);
static int		mxml_string_putc(int ch, void *p);
static int		mxml_write_node(stp_mxml_node_t *node, void *p,
			                int (*cb)(stp_mxml_node_t *, int),
					int col,
					int (*putc_cb)(int, void *));
static int		mxml_write_string(const char *s, void *p,
					  int (*putc_cb)(int, void *));
static int		mxml_write_ws(stp_mxml_node_t *node, void *p, 
			              int (*cb)(stp_mxml_node_t *, int), int ws,
				      int col, int (*putc_cb)(int, void *));


/*
 * 'stp_mxmlLoadFile()' - Load a file into an XML node tree.
 *
 * The nodes in the specified file are added to the specified top node.
 * If no top node is provided, the XML file MUST be well-formed with a
 * single parent node like <?xml> for the entire file. The callback
 * function returns the value type that should be used for child nodes.
 * If STP_MXML_NO_CALLBACK is specified then all child nodes will be either
 * STP_MXML_ELEMENT or STP_MXML_TEXT nodes.
 */

stp_mxml_node_t *				/* O - First node or NULL if the file could not be read. */
stp_mxmlLoadFile(stp_mxml_node_t *top,		/* I - Top node */
             FILE        *fp,		/* I - File to read from */
             stp_mxml_type_t (*cb)(stp_mxml_node_t *))
					/* I - Callback function or STP_MXML_NO_CALLBACK */
{
  return (mxml_load_data(top, fp, cb, mxml_file_getc));
}

/*
 * 'stp_mxmlLoadFromFile()' - Load a named file into an XML node tree.
 *
 * The nodes in the specified file are added to the specified top node.
 * If no top node is provided, the XML file MUST be well-formed with a
 * single parent node like <?xml> for the entire file. The callback
 * function returns the value type that should be used for child nodes.
 * If STP_MXML_NO_CALLBACK is specified then all child nodes will be either
 * STP_MXML_ELEMENT or STP_MXML_TEXT nodes.
 */

stp_mxml_node_t *				/* O - First node or NULL if the file could not be read. */
stp_mxmlLoadFromFile(stp_mxml_node_t *top,	/* I - Top node */
		     const char *file,		/* I - File to read from */
		     stp_mxml_type_t (*cb)(stp_mxml_node_t *))
					/* I - Callback function or STP_MXML_NO_CALLBACK */
{
  FILE *fp = fopen(file, "r");
  stp_mxml_node_t *doc;
  if (! fp)
    return NULL;
  doc = stp_mxmlLoadFile(top, fp, cb);
  fclose(fp);
  return doc;
}


/*
 * 'stp_mxmlLoadString()' - Load a string into an XML node tree.
 *
 * The nodes in the specified string are added to the specified top node.
 * If no top node is provided, the XML string MUST be well-formed with a
 * single parent node like <?xml> for the entire string. The callback
 * function returns the value type that should be used for child nodes.
 * If STP_MXML_NO_CALLBACK is specified then all child nodes will be either
 * STP_MXML_ELEMENT or STP_MXML_TEXT nodes.
 */

stp_mxml_node_t *				/* O - First node or NULL if the string has errors. */
stp_mxmlLoadString(stp_mxml_node_t *top,	/* I - Top node */
               const char  *s,		/* I - String to load */
               stp_mxml_type_t (*cb)(stp_mxml_node_t *))
					/* I - Callback function or STP_MXML_NO_CALLBACK */
{
  return (mxml_load_data(top, &s, cb, mxml_string_getc));
}


/*
 * 'stp_mxmlSaveAllocString()' - Save an XML node tree to an allocated string.
 *
 * This function returns a pointer to a string containing the textual
 * representation of the XML node tree.  The string should be freed
 * using the free() function when you are done with it.  NULL is returned
 * if the node would produce an empty string or if the string cannot be
 * allocated.
 */

char *					/* O - Allocated string or NULL */
stp_mxmlSaveAllocString(stp_mxml_node_t *node,	/* I - Node to write */
                    int         (*cb)(stp_mxml_node_t *, int))
					/* I - Whitespace callback or STP_MXML_NO_CALLBACK */
{
  int	bytes;				/* Required bytes */
  char	buffer[8192];			/* Temporary buffer */
  char	*s;				/* Allocated string */


 /*
  * Write the node to the temporary buffer...
  */

  bytes = stp_mxmlSaveString(node, buffer, sizeof(buffer), cb);

  if (bytes <= 0)
    return (NULL);

  if (bytes < (int)(sizeof(buffer) - 1))
  {
   /*
    * Node fit inside the buffer, so just duplicate that string and
    * return...
    */

    return (strdup(buffer));
  }

 /*
  * Allocate a buffer of the required size and save the node to the
  * new buffer...
  */

  if ((s = malloc(bytes + 1)) == NULL)
    return (NULL);

  stp_mxmlSaveString(node, s, bytes + 1, cb);

 /*
  * Return the allocated string...
  */

  return (s);
}


/*
 * 'stp_mxmlSaveFile()' - Save an XML tree to a file.
 *
 * The callback argument specifies a function that returns a whitespace
 * character or nul (0) before and after each element. If STP_MXML_NO_CALLBACK
 * is specified, whitespace will only be added before STP_MXML_TEXT nodes
 * with leading whitespace and before attribute names inside opening
 * element tags.
 */

int					/* O - 0 on success, -1 on error. */
stp_mxmlSaveFile(stp_mxml_node_t *node,		/* I - Node to write */
             FILE        *fp,		/* I - File to write to */
	     int         (*cb)(stp_mxml_node_t *, int))
					/* I - Whitespace callback or STP_MXML_NO_CALLBACK */
{
  int	col;				/* Final column */


 /*
  * Write the node...
  */

  if ((col = mxml_write_node(node, fp, cb, 0, mxml_file_putc)) < 0)
    return (-1);

  if (col > 0)
    if (putc('\n', fp) < 0)
      return (-1);

 /*
  * Return 0 (success)...
  */

  return (0);
}

int					/* O - 0 on success, -1 on error. */
stp_mxmlSaveToFile(stp_mxml_node_t *node,	/* I - Node to write */
		   const char      *file,	/* I - File to write to */
		   int             (*cb)(stp_mxml_node_t *, int))
					/* I - Whitespace callback or STP_MXML_NO_CALLBACK */
{
  FILE *fp = fopen(file, "w");
  int answer;
  int status;
  if (!fp)
    return -1;
  answer = stp_mxmlSaveFile(node, fp, cb);
  status = fclose(fp);
  if (status != 0)
    return -1;
  else
    return answer;
}

/*
 * 'stp_mxmlSaveString()' - Save an XML node tree to a string.
 *
 * This function returns the total number of bytes that would be
 * required for the string but only copies (bufsize - 1) characters
 * into the specified buffer.
 */

int					/* O - Size of string */
stp_mxmlSaveString(stp_mxml_node_t *node,	/* I - Node to write */
               char        *buffer,	/* I - String buffer */
               int         bufsize,	/* I - Size of string buffer */
               int         (*cb)(stp_mxml_node_t *, int))
					/* I - Whitespace callback or STP_MXML_NO_CALLBACK */
{
  int	col;				/* Final column */
  char	*ptr[2];			/* Pointers for putc_cb */


 /*
  * Write the node...
  */

  ptr[0] = buffer;
  ptr[1] = buffer + bufsize;

  if ((col = mxml_write_node(node, ptr, cb, 0, mxml_string_putc)) < 0)
    return (-1);

  if (col > 0)
    mxml_string_putc('\n', ptr);

 /*
  * Nul-terminate the buffer...
  */

  if (ptr[0] >= ptr[1])
    buffer[bufsize - 1] = '\0';
  else
    ptr[0][0] = '\0';

 /*
  * Return the number of characters...
  */

  return (ptr[0] - buffer);
}


/*
 * 'mxml_add_char()' - Add a character to a buffer, expanding as needed.
 */

static int				/* O  - 0 on success, -1 on error */
mxml_add_char(int  ch,			/* I  - Character to add */
              char **bufptr,		/* IO - Current position in buffer */
	      char **buffer,		/* IO - Current buffer */
	      int  *bufsize)		/* IO - Current buffer size */
{
  char	*newbuffer;			/* New buffer value */


  if (*bufptr >= (*buffer + *bufsize - 1))
  {
   /*
    * Increase the size of the buffer...
    */

    if (*bufsize < 1024)
      (*bufsize) *= 2;
    else
      (*bufsize) += 1024;

    if ((newbuffer = realloc(*buffer, *bufsize)) == NULL)
    {
      free(*buffer);

      fprintf(stderr, "Unable to expand string buffer to %d bytes!\n",
	      *bufsize);

      return (-1);
    }

    *bufptr = newbuffer + (*bufptr - *buffer);
    *buffer = newbuffer;
  }

  *(*bufptr)++ = ch;

  return (0);
}


/*
 * 'mxml_file_getc()' - Get a character from a file.
 */

static int				/* O - Character or EOF */
mxml_file_getc(void *p)			/* I - Pointer to file */
{
  return (getc((FILE *)p));
}


/*
 * 'mxml_file_putc()' - Write a character to a file.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_file_putc(int  ch,			/* I - Character to write */
               void *p)			/* I - Pointer to file */
{
  return (putc(ch, (FILE *)p));
}


/*
 * 'mxml_load_data()' - Load data into an XML node tree.
 */

static stp_mxml_node_t *			/* O - First node or NULL if the file could not be read. */
mxml_load_data(stp_mxml_node_t *top,	/* I - Top node */
               void        *p,		/* I - Pointer to data */
               stp_mxml_type_t (*cb)(stp_mxml_node_t *),
					/* I - Callback function or STP_MXML_NO_CALLBACK */
               int         (*getc_cb)(void *))
					/* I - Read function */
{
  stp_mxml_node_t	*node,			/* Current node */
		*parent;		/* Current parent node */
  int		ch,			/* Character from file */
		whitespace;		/* Non-zero if whitespace seen */
  char		*buffer,		/* String buffer */
		*bufptr;		/* Pointer into buffer */
  int		bufsize;		/* Size of buffer */
  stp_mxml_type_t	type;			/* Current node type */


 /*
  * Read elements and other nodes from the file...
  */

  if ((buffer = malloc(64)) == NULL)
  {
    fputs("Unable to allocate string buffer!\n", stderr);
    return (NULL);
  }

  bufsize    = 64;
  bufptr     = buffer;
  parent     = top;
  whitespace = 0;

  if (cb && parent)
    type = (*cb)(parent);
  else
    type = STP_MXML_TEXT;

  while ((ch = (*getc_cb)(p)) != EOF)
  {
    if ((ch == '<' || (isspace(ch) && type != STP_MXML_OPAQUE)) && bufptr > buffer)
    {
     /*
      * Add a new value node...
      */

      *bufptr = '\0';

      switch (type)
      {
	case STP_MXML_INTEGER :
            node = stp_mxmlNewInteger(parent, strtol(buffer, &bufptr, 0));
	    break;

	case STP_MXML_OPAQUE :
            node = stp_mxmlNewOpaque(parent, buffer);
	    break;

	case STP_MXML_REAL :
            node = stp_mxmlNewReal(parent, strtod(buffer, &bufptr));
	    break;

	case STP_MXML_TEXT :
            node = stp_mxmlNewText(parent, whitespace, buffer);
	    break;

        default : /* Should never happen... */
	    node = NULL;
	    break;
      }	  

      if (*bufptr)
      {
       /*
        * Bad integer/real number value...
	*/

        fprintf(stderr, "Bad %s value '%s' in parent <%s>!\n",
	        type == STP_MXML_INTEGER ? "integer" : "real", buffer,
		parent ? parent->value.element.name : "null");
	break;
      }

      bufptr     = buffer;
      whitespace = isspace(ch) && type == STP_MXML_TEXT;

      if (!node)
      {
       /*
	* Just print error for now...
	*/

	fprintf(stderr, "Unable to add value node of type %d to parent <%s>!\n",
	        type, parent ? parent->value.element.name : "null");
	break;
      }
    }
    else if (isspace(ch) && type == STP_MXML_TEXT)
      whitespace = 1;

   /*
    * Add lone whitespace node if we have an element and existing
    * whitespace...
    */

    if (ch == '<' && whitespace && type == STP_MXML_TEXT)
    {
      stp_mxmlNewText(parent, whitespace, "");
      whitespace = 0;
    }

    if (ch == '<')
    {
     /*
      * Start of open/close tag...
      */

      bufptr = buffer;

      while ((ch = (*getc_cb)(p)) != EOF)
        if (isspace(ch) || ch == '>' || (ch == '/' && bufptr > buffer))
	  break;
	else if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	{
          free(buffer);
	  return (NULL);
	}
	else if ((bufptr - buffer) == 3 && !strncmp(buffer, "!--", 3))
	  break;

      *bufptr = '\0';

      if (!strcmp(buffer, "!--"))
      {
       /*
        * Gather rest of comment...
	*/

	while ((ch = (*getc_cb)(p)) != EOF)
	{
	  if (ch == '>' && bufptr > (buffer + 4) &&
	      !strncmp(bufptr - 2, "--", 2))
	    break;
	  else if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	}

       /*
        * Error out if we didn't get the whole comment...
	*/

        if (ch != '>')
	  break;

       /*
        * Otherwise add this as an element under the current parent...
	*/

	*bufptr = '\0';

	if (!stp_mxmlNewElement(parent, buffer))
	{
	 /*
	  * Just print error for now...
	  */

	  fprintf(stderr, "Unable to add comment node to parent <%s>!\n",
	          parent ? parent->value.element.name : "null");
	  break;
	}
      }
      else if (buffer[0] == '!')
      {
       /*
        * Gather rest of declaration...
	*/

	do
	{
	  if (ch == '>')
	    break;
	  else if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	}
        while ((ch = (*getc_cb)(p)) != EOF);

       /*
        * Error out if we didn't get the whole declaration...
	*/

        if (ch != '>')
	  break;

       /*
        * Otherwise add this as an element under the current parent...
	*/

	*bufptr = '\0';

	node = stp_mxmlNewElement(parent, buffer);
	if (!node)
	{
	 /*
	  * Just print error for now...
	  */

	  fprintf(stderr, "Unable to add declaration node to parent <%s>!\n",
	          parent ? parent->value.element.name : "null");
	  break;
	}

       /*
	* Descend into this node, setting the value type as needed...
	*/

	parent = node;

	if (cb && parent)
	  type = (*cb)(parent);
      }
      else if (buffer[0] == '/')
      {
       /*
        * Handle close tag...
	*/

        if (!parent || strcmp(buffer + 1, parent->value.element.name))
	{
	 /*
	  * Close tag doesn't match tree; print an error for now...
	  */

	  fprintf(stderr, "Mismatched close tag <%s> under parent <%s>!\n",
	          buffer, parent ? parent->value.element.name : "(null)");
          break;
	}

       /*
        * Keep reading until we see >...
	*/

        while (ch != '>' && ch != EOF)
	  ch = (*getc_cb)(p);

       /*
	* Ascend into the parent and set the value type as needed...
	*/

	parent = parent->parent;

	if (cb && parent)
	  type = (*cb)(parent);
      }
      else
      {
       /*
        * Handle open tag...
	*/

        node = stp_mxmlNewElement(parent, buffer);

	if (!node)
	{
	 /*
	  * Just print error for now...
	  */

	  fprintf(stderr, "Unable to add element node to parent <%s>!\n",
	          parent ? parent->value.element.name : "null");
	  break;
	}

        if (isspace(ch))
          ch = mxml_parse_element(node, p, getc_cb);
        else if (ch == '/')
	{
	  if ((ch = (*getc_cb)(p)) != '>')
	  {
	    fprintf(stderr, "Expected > but got '%c' instead for element <%s/>!\n",
	            ch, buffer);
            break;
	  }

	  ch = '/';
	}

	if (ch == EOF)
	  break;

        if (ch != '/')
	{
	 /*
	  * Descend into this node, setting the value type as needed...
	  */

	  parent = node;

	  if (cb && parent)
	    type = (*cb)(parent);
	}
      }

      bufptr  = buffer;
    }
    else if (ch == '&')
    {
     /*
      * Add character entity to current buffer...  Currently we only
      * support &lt;, &amp;, &gt;, &nbsp;, &quot;, &#nnn;, and &#xXXXX;...
      */

      char	entity[64],		/* Entity string */
		*entptr;		/* Pointer into entity */


      entity[0] = ch;
      entptr    = entity + 1;

      while ((ch = (*getc_cb)(p)) != EOF)
        if (!isalnum(ch) && ch != '#')
	  break;
	else if (entptr < (entity + sizeof(entity) - 1))
	  *entptr++ = ch;
	else
	{
	  fprintf(stderr, "Entity name too long under parent <%s>!\n",
	          parent ? parent->value.element.name : "null");
          break;
	}

      *entptr = '\0';

      if (ch != ';')
      {
	fprintf(stderr, "Entity name \"%s\" not terminated under parent <%s>!\n",
	        entity, parent ? parent->value.element.name : "null");
        break;
      }

      if (entity[1] == '#')
      {
	if (entity[2] == 'x')
	  ch = strtol(entity + 3, NULL, 16);
	else
	  ch = strtol(entity + 2, NULL, 10);
      }
      else if (!strcmp(entity, "&amp"))
        ch = '&';
      else if (!strcmp(entity, "&gt"))
        ch = '>';
      else if (!strcmp(entity, "&lt"))
        ch = '<';
      else if (!strcmp(entity, "&nbsp"))
        ch = 0xa0;
      else if (!strcmp(entity, "&quot"))
        ch = '\"';
      else
      {
	fprintf(stderr, "Entity name \"%s;\" not supported under parent <%s>!\n",
	        entity, parent ? parent->value.element.name : "null");
        break;
      }

      if (ch < 128)
      {
       /*
        * Plain ASCII doesn't need special encoding...
	*/

	if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	{
          free(buffer);
	  return (NULL);
	}
      }
      else
      {
       /*
        * Use UTF-8 encoding for the Unicode char...
	*/

	if (ch < 2048)
	{
	  if (mxml_add_char(0xc0 | (ch >> 6), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | (ch & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
        }
	else if (ch < 65536)
	{
	  if (mxml_add_char(0xe0 | (ch >> 12), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | ((ch >> 6) & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | (ch & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	}
	else
	{
	  if (mxml_add_char(0xf0 | (ch >> 18), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | ((ch >> 12) & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | ((ch >> 6) & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	  if (mxml_add_char(0x80 | (ch & 63), &bufptr, &buffer, &bufsize))
	  {
            free(buffer);
	    return (NULL);
	  }
	}
      }
    }
    else if (type == STP_MXML_OPAQUE || !isspace(ch))
    {
     /*
      * Add character to current buffer...
      */

      if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
      {
        free(buffer);
	return (NULL);
      }
    }
  }

 /*
  * Free the string buffer - we don't need it anymore...
  */

  free(buffer);

 /*
  * Find the top element and return it...
  */

  if (parent)
  {
    while (parent->parent != top)
      parent = parent->parent;
  }

  return (parent);
}


/*
 * 'mxml_parse_element()' - Parse an element for any attributes...
 */

static int				/* O - Terminating character */
mxml_parse_element(stp_mxml_node_t *node,	/* I - Element node */
                   void        *p,	/* I - Data to read from */
                   int         (*getc_cb)(void *))
					/* I - Data callback */
{
  int	ch,				/* Current character in file */
	quote;				/* Quoting character */
  char	*name,				/* Attribute name */
	*value,				/* Attribute value */
	*ptr;				/* Pointer into name/value */
  int	namesize,			/* Size of name string */
	valsize;			/* Size of value string */


 /*
  * Initialize the name and value buffers...
  */

  if ((name = malloc(64)) == NULL)
  {
    fputs("Unable to allocate memory for name!\n", stderr);
    return (EOF);
  }

  namesize = 64;

  if ((value = malloc(64)) == NULL)
  {
    free(name);
    fputs("Unable to allocate memory for value!\n", stderr);
    return (EOF);
  }

  valsize = 64;

 /*
  * Loop until we hit a >, /, ?, or EOF...
  */

  while ((ch = (*getc_cb)(p)) != EOF)
  {
#ifdef DEBUG
    fprintf(stderr, "parse_element: ch='%c'\n", ch);
#endif /* DEBUG */

   /*
    * Skip leading whitespace...
    */

    if (isspace(ch))
      continue;

   /*
    * Stop at /, ?, or >...
    */

    if (ch == '/' || ch == '?')
    {
     /*
      * Grab the > character and print an error if it isn't there...
      */

      quote = (*getc_cb)(p);

      if (quote != '>')
      {
        fprintf(stderr, "Expected '>' after '%c' for element %s, but got '%c'!\n",
	        ch, node->value.element.name, quote);
        ch = EOF;
      }

      break;
    }
    else if (ch == '>')
      break;

   /*
    * Read the attribute name...
    */

    name[0] = ch;
    ptr     = name + 1;

    while ((ch = (*getc_cb)(p)) != EOF)
      if (isspace(ch) || ch == '=' || ch == '/' || ch == '>' || ch == '?')
        break;
      else if (mxml_add_char(ch, &ptr, &name, &namesize))
      {
        free(name);
	free(value);
	return (EOF);
      }

    *ptr = '\0';

    if (ch == '=')
    {
     /*
      * Read the attribute value...
      */

      if ((ch = (*getc_cb)(p)) == EOF)
      {
        fprintf(stderr, "Missing value for attribute '%s' in element %s!\n",
	        name, node->value.element.name);
        return (EOF);
      }

      if (ch == '\'' || ch == '\"')
      {
       /*
        * Read quoted value...
	*/

        quote = ch;
	ptr   = value;

        while ((ch = (*getc_cb)(p)) != EOF)
	  if (ch == quote)
	    break;
	  else if (mxml_add_char(ch, &ptr, &value, &valsize))
	  {
            free(name);
	    free(value);
	    return (EOF);
	  }

        *ptr = '\0';
      }
      else
      {
       /*
        * Read unquoted value...
	*/

	value[0] = ch;
	ptr      = value + 1;

	while ((ch = (*getc_cb)(p)) != EOF)
	  if (isspace(ch) || ch == '=' || ch == '/' || ch == '>')
            break;
	  else if (mxml_add_char(ch, &ptr, &value, &valsize))
	  {
            free(name);
	    free(value);
	    return (EOF);
	  }

        *ptr = '\0';
      }
    }
    else
      value[0] = '\0';

   /*
    * Save last character in case we need it...
    */

    if (ch == '/' || ch == '?')
    {
     /*
      * Grab the > character and print an error if it isn't there...
      */

      quote = (*getc_cb)(p);

      if (quote != '>')
      {
        fprintf(stderr, "Expected '>' after '%c' for element %s, but got '%c'!\n",
	        ch, node->value.element.name, quote);
        ch = EOF;
      }

      break;
    }
    else if (ch == '>')
      break;

   /*
    * Set the attribute...
    */

    stp_mxmlElementSetAttr(node, name, value);
  }

 /*
  * Free the name and value buffers and return...
  */

  free(name);
  free(value);

  return (ch);
}


/*
 * 'mxml_string_getc()' - Get a character from a string.
 */

static int				/* O - Character or EOF */
mxml_string_getc(void *p)		/* I - Pointer to file */
{
  int		ch;			/* Character */
  const char	**s;			/* Pointer to string pointer */


  s = (const char **)p;

  if ((ch = *s[0]) != 0)
  {
    (*s)++;
    return (ch);
  }
  else
    return (EOF);
}


/*
 * 'mxml_string_putc()' - Write a character to a string.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_string_putc(int  ch,		/* I - Character to write */
                 void *p)		/* I - Pointer to string pointers */
{
  char	**pp;				/* Pointer to string pointers */


  pp = (char **)p;

  if (pp[0] < pp[1])
    pp[0][0] = ch;

  pp[0] ++;

  return (0);
}


/*
 * 'mxml_write_node()' - Save an XML node to a file.
 */

static int				/* O - Column or -1 on error */
mxml_write_node(stp_mxml_node_t *node,	/* I - Node to write */
                void        *p,		/* I - File to write to */
	        int         (*cb)(stp_mxml_node_t *, int),
					/* I - Whitespace callback */
		int         col,	/* I - Current column */
		int         (*putc_cb)(int, void *))
{
  int		i;			/* Looping var */
  stp_mxml_attr_t	*attr;			/* Current attribute */
  char		s[255];			/* Temporary string */


  while (node != NULL)
  {
   /*
    * Print the node value...
    */

    switch (node->type)
    {
      case STP_MXML_ELEMENT :
          col = mxml_write_ws(node, p, cb, STP_MXML_WS_BEFORE_OPEN, col, putc_cb);

          if ((*putc_cb)('<', p) < 0)
	    return (-1);
	  if (mxml_write_string(node->value.element.name, p, putc_cb) < 0)
	    return (-1);

          col += strlen(node->value.element.name) + 1;

	  for (i = node->value.element.num_attrs, attr = node->value.element.attrs;
	       i > 0;
	       i --, attr ++)
	  {
	    if ((col + strlen(attr->name) + strlen(attr->value) + 3) > STP_MXML_WRAP)
	    {
	      if ((*putc_cb)('\n', p) < 0)
	        return (-1);

	      col = 0;
	    }
	    else
	    {
	      if ((*putc_cb)(' ', p) < 0)
	        return (-1);

	      col ++;
	    }

            if (mxml_write_string(attr->name, p, putc_cb) < 0)
	      return (-1);
            if ((*putc_cb)('=', p) < 0)
	      return (-1);
            if ((*putc_cb)('\"', p) < 0)
	      return (-1);
	    if (mxml_write_string(attr->value, p, putc_cb) < 0)
	      return (-1);
            if ((*putc_cb)('\"', p) < 0)
	      return (-1);
	    
            col += strlen(attr->name) + strlen(attr->value) + 3;
	  }

	  if (node->child)
	  {
           /*
	    * The ? and ! elements are special-cases and have no end tags...
	    */

	    if (node->value.element.name[0] == '?')
	    {
              if ((*putc_cb)('?', p) < 0)
		return (-1);
              if ((*putc_cb)('>', p) < 0)
		return (-1);
              if ((*putc_cb)('\n', p) < 0)
		return (-1);

              col = 0;
            }
	    else if ((*putc_cb)('>', p) < 0)
	      return (-1);
	    else
	      col ++;

            col = mxml_write_ws(node, p, cb, STP_MXML_WS_AFTER_OPEN, col, putc_cb);

	    if ((col = mxml_write_node(node->child, p, cb, col, putc_cb)) < 0)
	      return (-1);

            if (node->value.element.name[0] != '?' &&
	        node->value.element.name[0] != '!')
	    {
              col = mxml_write_ws(node, p, cb, STP_MXML_WS_BEFORE_CLOSE, col, putc_cb);

              if ((*putc_cb)('<', p) < 0)
		return (-1);
              if ((*putc_cb)('/', p) < 0)
		return (-1);
              if (mxml_write_string(node->value.element.name, p, putc_cb) < 0)
		return (-1);
              if ((*putc_cb)('>', p) < 0)
		return (-1);

              col += strlen(node->value.element.name) + 3;

              col = mxml_write_ws(node, p, cb, STP_MXML_WS_AFTER_CLOSE, col, putc_cb);
	    }
	  }
	  else if (node->value.element.name[0] == '!')
	  {
	    if ((*putc_cb)('>', p) < 0)
	      return (-1);
	    else
	      col ++;

            col = mxml_write_ws(node, p, cb, STP_MXML_WS_AFTER_OPEN, col, putc_cb);
          }
	  else
	  {
            if ((*putc_cb)('/', p) < 0)
	      return (-1);
            if ((*putc_cb)('>', p) < 0)
	      return (-1);

	    col += 2;

            col = mxml_write_ws(node, p, cb, STP_MXML_WS_AFTER_OPEN, col, putc_cb);
	  }
          break;

      case STP_MXML_INTEGER :
	  if (node->prev)
	  {
	    if (col > STP_MXML_WRAP)
	    {
	      if ((*putc_cb)('\n', p) < 0)
	        return (-1);

	      col = 0;
	    }
	    else if ((*putc_cb)(' ', p) < 0)
	      return (-1);
	    else
	      col ++;
          }

          sprintf(s, "%d", node->value.integer);
	  if (mxml_write_string(s, p, putc_cb) < 0)
	    return (-1);

	  col += strlen(s);
          break;

      case STP_MXML_OPAQUE :
          if (mxml_write_string(node->value.opaque, p, putc_cb) < 0)
	    return (-1);

          col += strlen(node->value.opaque);
          break;

      case STP_MXML_REAL :
	  if (node->prev)
	  {
	    if (col > STP_MXML_WRAP)
	    {
	      if ((*putc_cb)('\n', p) < 0)
	        return (-1);

	      col = 0;
	    }
	    else if ((*putc_cb)(' ', p) < 0)
	      return (-1);
	    else
	      col ++;
          }

          sprintf(s, "%f", node->value.real);
	  if (mxml_write_string(s, p, putc_cb) < 0)
	    return (-1);

	  col += strlen(s);
          break;

      case STP_MXML_TEXT :
	  if (node->value.text.whitespace && col > 0)
	  {
	    if (col > STP_MXML_WRAP)
	    {
	      if ((*putc_cb)('\n', p) < 0)
	        return (-1);

	      col = 0;
	    }
	    else if ((*putc_cb)(' ', p) < 0)
	      return (-1);
	    else
	      col ++;
          }

          if (mxml_write_string(node->value.text.string, p, putc_cb) < 0)
	    return (-1);

	  col += strlen(node->value.text.string);
          break;
    }

   /*
    * Next node...
    */

    node = node->next;
  }

  return (col);
}


/*
 * 'mxml_write_string()' - Write a string, escaping & and < as needed.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_write_string(const char *s,	/* I - String to write */
                  void       *p,	/* I - Write pointer */
		  int        (*putc_cb)(int, void *))
					/* I - Write callback */
{
  char	buf[255],			/* Buffer */
	*bufptr;			/* Pointer into buffer */


  while (*s)
  {
    if (*s == '&')
    {
      if ((*putc_cb)('&', p) < 0)
        return (-1);
      if ((*putc_cb)('a', p) < 0)
        return (-1);
      if ((*putc_cb)('m', p) < 0)
        return (-1);
      if ((*putc_cb)('p', p) < 0)
        return (-1);
      if ((*putc_cb)(';', p) < 0)
        return (-1);
    }
    else if (*s == '<')
    {
      if ((*putc_cb)('&', p) < 0)
        return (-1);
      if ((*putc_cb)('l', p) < 0)
        return (-1);
      if ((*putc_cb)('t', p) < 0)
        return (-1);
      if ((*putc_cb)(';', p) < 0)
        return (-1);
    }
    else if (*s == '>')
    {
      if ((*putc_cb)('&', p) < 0)
        return (-1);
      if ((*putc_cb)('g', p) < 0)
        return (-1);
      if ((*putc_cb)('t', p) < 0)
        return (-1);
      if ((*putc_cb)(';', p) < 0)
        return (-1);
    }
    else if (*s == '\"')
    {
      if ((*putc_cb)('&', p) < 0)
        return (-1);
      if ((*putc_cb)('q', p) < 0)
        return (-1);
      if ((*putc_cb)('u', p) < 0)
        return (-1);
      if ((*putc_cb)('o', p) < 0)
        return (-1);
      if ((*putc_cb)('t', p) < 0)
        return (-1);
      if ((*putc_cb)(';', p) < 0)
        return (-1);
    }
    else if (*s & 128)
    {
     /*
      * Convert UTF-8 to Unicode constant...
      */

      int	ch;			/* Unicode character */


      ch = *s & 255;

      if ((ch & 0xe0) == 0xc0)
      {
        ch = ((ch & 0x1f) << 6) | (s[1] & 0x3f);
	s ++;
      }
      else if ((ch & 0xf0) == 0xe0)
      {
        ch = ((((ch * 0x0f) << 6) | (s[1] & 0x3f)) << 6) | (s[2] & 0x3f);
	s += 2;
      }

      if (ch == 0xa0)
      {
       /*
        * Handle non-breaking space as-is...
	*/

	if ((*putc_cb)('&', p) < 0)
          return (-1);
	if ((*putc_cb)('n', p) < 0)
          return (-1);
	if ((*putc_cb)('b', p) < 0)
          return (-1);
	if ((*putc_cb)('s', p) < 0)
          return (-1);
	if ((*putc_cb)('p', p) < 0)
          return (-1);
	if ((*putc_cb)(';', p) < 0)
          return (-1);
      }
      else
      {
        sprintf(buf, "&#x%x;", ch);

	for (bufptr = buf; *bufptr; bufptr ++)
	  if ((*putc_cb)(*bufptr, p) < 0)
	    return (-1);
      }
    }
    else if ((*putc_cb)(*s, p) < 0)
      return (-1);

    s ++;
  }

  return (0);
}


/*
 * 'mxml_write_ws()' - Do whitespace callback...
 */

static int				/* O - New column */
mxml_write_ws(stp_mxml_node_t *node,	/* I - Current node */
              void        *p,		/* I - Write pointer */
              int         (*cb)(stp_mxml_node_t *, int),
					/* I - Callback function */
	      int         ws,		/* I - Where value */
	      int         col,		/* I - Current column */
              int         (*putc_cb)(int, void *))
					/* I - Write callback */
{
  int	ch;				/* Whitespace character */


  if (cb && (ch = (*cb)(node, ws)) != 0)
  {
    if ((*putc_cb)(ch, p) < 0)
      return (-1);
    else if (ch == '\n')
      col = 0;
    else if (ch == '\t')
    {
      col += STP_MXML_TAB;
      col = col - (col % STP_MXML_TAB);
    }
    else
      col ++;
  }

  return (col);
}


/*
 * End of "$Id: mxml-file.c,v 1.10 2008/07/20 01:12:15 easysw Exp $".
 */
