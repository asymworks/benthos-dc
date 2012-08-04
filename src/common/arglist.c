/*
 * Copyright (C) 2012 Asymworks, LLC.  All Rights Reserved.
 * www.asymworks.com / info@asymworks.com
 *
 * This file is part of the Benthos Dive Log Package (benthos-log.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arglist.h"

/*
 * Argument Linked-List Entry
 */
struct arglist_
{
	const char *		name;
	const char *		value;
	struct arglist_	*	next;
};

int arglist_parse(arglist_t * arglist, const char * argstring)
{
	if (arglist == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	arglist_t head = 0;
	arglist_t cur = 0;
	ssize_t p;
	ssize_t l = strlen(argstring);

	int in_name;
	char name[258];		ssize_t nsize;
	char value[258];	ssize_t vsize;

	memset(name, '\0', 258);
	memset(value, '\0', 258);

	// In case there is no argument string to parse
	* arglist = NULL;

	// Initialize the Scan
	in_name = 1;
	nsize = 0;
	vsize = 0;

	name[0] = '\0';
	value[0] = '\0';

	// Scan through the argument string
	for (p = 0; p < l; ++p)
	{
		if (argstring[p] == '=')
		{
			if ((p < (l-1)) && (argstring[p+1] == '='))
			{
				// Doubled Equals Sign
				if (in_name)
					name[nsize++] = '=';
				else
					value[vsize++] = '=';

				++p;
			}
			else
			{
				// Shift from Name to Value
				if (! in_name)
				{
					// Should not have a standalone equals sign in the value portion
					errno = EINVAL;
					return -1;
				}

				name[nsize++] = '\0';
				in_name = 0;
			}
		}
		else if (argstring[p] == ':')
		{
			if ((p < (l-1)) && (argstring[p+1] == ':'))
			{
				// Doubled Colon
				if (in_name)
					name[nsize++] = ':';
				else
					value[vsize++] = ':';

				++p;
			}
			else
			{
				// End of Name/Value Token Pair
				if (in_name)
					name[nsize++] = '\0';
				else
					value[vsize++] = '\0';

				// Create the Entry
				arglist_t entry = (arglist_t)malloc(sizeof(struct arglist_));
				if (entry == NULL)
					return -1;

				entry->name = strdup(name);
				entry->value = strdup(value);
				entry->next = 0;

				// Link to the List
				if (head == NULL)
				{
					head = entry;
					cur = head;
				}
				else
				{
					cur->next = entry;
					cur = entry;
				}

				nsize = 0;	memset(name, '\0', 258);
				vsize = 0;	memset(value, '\0', 258);
				in_name = 1;
			}
		}
		else
		{
			if (in_name)
				name[nsize++] = argstring[p];
			else
				value[vsize++] = argstring[p];
		}
	}

	// If the string did not end in a colon, add the last entry
	arglist_t entry = (arglist_t)malloc(sizeof(struct arglist_));
	if (entry == NULL)
		return -1;

	entry->name = strdup(name);
	entry->value = strdup(value);
	entry->next = 0;

	// Link to the List
	if (head == NULL)
	{
		head = entry;
		cur = head;
	}
	else
	{
		cur->next = entry;
		cur = entry;
	}

	// Finished
	* arglist = head;
	return 0;
}

void arglist_close(arglist_t args)
{
	arglist_t cur = args;
	while (cur != NULL)
	{
		arglist_t next = cur->next;
		free(cur);
		cur = next;
	}
}

ssize_t arglist_count(arglist_t args)
{
	ssize_t c = 0;
	arglist_t entry = args;
	while (entry != NULL)
	{
		++c;
		entry = entry->next;
	}

	return c;
}

int arglist_has(arglist_t args, const char * name)
{
	arglist_t entry = args;
	while (entry != NULL)
	{
		if (strcmp(entry->name, name) == 0)
		{
			return 1;
		}

		entry = entry->next;
	}

	return 0;
}

int arglist_hasvalue(arglist_t args, const char * name)
{
	arglist_t entry = args;
	while (entry != NULL)
	{
		if (strcmp(entry->name, name) == 0)
		{
			return (entry->value != NULL) || (strlen(entry->value) == 0);
		}

		entry = entry->next;
	}

	return 0;
}

int arglist_read_string(arglist_t args, const char * name, const char ** value)
{
	arglist_t entry = args;
	while (entry != NULL)
	{
		if (strcmp(entry->name, name) == 0)
		{
			if ((entry->value == NULL) || (strlen(entry->value) == 0))
			{
				return 1;
			}
			else
			{
				* value = entry->value;
				return 0;
			}
		}

		entry = entry->next;
	}

	return 2;
}

int arglist_read_int(arglist_t args, const char * name, int32_t * value)
{
	const char * sval;
	int rc = arglist_read_string(args, name, & sval);
	if (rc != 0)
		return rc;

	rc = sscanf(sval, "%d", value);
	if (rc != 1)
		return 1;

	return 0;
}

int arglist_read_uint(arglist_t args, const char * name, uint32_t * value)
{
	const char * sval;
	int rc = arglist_read_string(args, name, & sval);
	if (rc != 0)
		return rc;

	rc = sscanf(sval, "%u", value);
	if (rc != 1)
		return 1;

	return 0;
}

int arglist_read_float(arglist_t args, const char * name, double * value)
{
	const char * sval;
	int rc = arglist_read_string(args, name, & sval);
	if (rc != 0)
		return rc;

	rc = sscanf(sval, "%lf", value);
	if (rc != 1)
		return 1;

	return 0;
}
