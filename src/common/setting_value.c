/*
 * Copyright (C) 2013 Asymworks, LLC.  All Rights Reserved.
 * www.asymworks.com / info@asymworks.com
 *
 * This file is part of the jkDiveLog Package (jkdivelog.com)
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
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "setting_value.h"

struct value_t
{
	union
	{
		int32_t				int_val;
		uint32_t			uint_val;
		int					bool_val;
		time_t				time_val;
		char *				str_val;
		struct value_t **	tuple_items;
	};

	int		type;
	int		size;
};

int value_type(value_handle_t v)
{
	return v->type;
}

int value_size(value_handle_t v)
{
	if (v->type != TYPE_TUPLE)
		return 1;

	return v->size;
}

void value_free(value_handle_t v)
{
	int i;

	if (! v)
		return;

	if (v->type == TYPE_ASCII)
		free(v->str_val);

	if (v->type == TYPE_TUPLE)
		for (i = 0; i < v->size; ++i)
			value_free(v->tuple_items[i]);

	free(v);
}

value_handle_t value_create_int(int32_t v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->int_val = v;
	ret->type = TYPE_INT;

	return ret;
}

value_handle_t value_create_uint(uint32_t v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->uint_val = v;
	ret->type = TYPE_UINT;

	return ret;
}

value_handle_t value_create_bool(int v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->bool_val = v;
	ret->type = TYPE_BOOL;

	return ret;
}

value_handle_t value_create_time(time_t v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->time_val = v;
	ret->type = TYPE_DATETIME;

	return ret;
}

value_handle_t value_create_str(const char * v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->str_val = strdup(v);
	ret->type = TYPE_ASCII;

	return ret;
}

value_handle_t value_create_tuple_ex(int size, value_handle_t * values)
{
	if (size < 0)
		return 0;

	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	memset(ret, 0, sizeof(struct value_t));
	ret->type = TYPE_TUPLE;
	ret->size = size;

	if (size == 0)
		return ret;

	ret->tuple_items = malloc(size * sizeof(struct value_t));

	if (! ret->tuple_items)
	{
		free(ret);
		return 0;
	}

	memset(ret->tuple_items, 0, size * sizeof(struct value_t));

	int i;
	for (i = 0; i < size; ++i)
	{
		ret->tuple_items[i] = value_clone(values[i]);
		if (! ret->tuple_items[i])
		{
			value_free(ret);
			return 0;
		}
	}

	return ret;
}

value_handle_t value_clone(value_handle_t v)
{
	value_handle_t ret = malloc(sizeof(struct value_t));
	if (! ret)
		return 0;

	if (v->type != TYPE_TUPLE)
	{
		memcpy(ret, v, sizeof(struct value_t));
		return ret;
	}

	ret->type = TYPE_TUPLE;
	ret->size = v->size;
	ret->tuple_items = malloc(v->size * sizeof(struct value_t));

	if (! ret->tuple_items)
	{
		free(ret);
		return 0;
	}

	memset(ret->tuple_items, 0, v->size * sizeof(struct value_t));

	int i;
	for (i = 0; i < v->size; ++i)
	{
		ret->tuple_items[i] = value_clone(v->tuple_items[i]);
		if (! ret->tuple_items[i])
		{
			value_free(ret);
			return 0;
		}
	}

	return ret;
}

int value_get_int(value_handle_t v, int32_t * out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_INT)
		return -2;

	*out = v->int_val;
	return 0;
}

int value_get_uint(value_handle_t v, uint32_t * out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_UINT)
		return -2;

	*out = v->uint_val;
	return 0;
}

int value_get_bool(value_handle_t v, int * out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_BOOL)
		return -2;

	*out = v->bool_val;
	return 0;
}

int value_get_time(value_handle_t v, time_t * out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_DATETIME)
		return -2;

	*out = v->int_val;
	return 0;
}

int value_get_str(value_handle_t v, char ** out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_ASCII)
		return -2;

	*out = strdup(v->str_val);
	return 0;
}

int value_get_tuple_item(value_handle_t v, int idx, value_handle_t * out)
{
	if (! v || ! out)
		return -1;

	if (v->type != TYPE_TUPLE)
		return -2;

	if ((idx < 0) || (idx >= v->size))
		return -3;

	memcpy(out, v->tuple_items[idx], sizeof(struct value_t));
	return 0;
}
